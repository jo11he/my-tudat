#ifndef CENTRALBODYDATA_H
#define CENTRALBODYDATA_H

#include <vector>
#include <map>
#include <iostream>

#include <boost/function.hpp>

#include <Eigen/Core>

namespace tudat
{

namespace propagators
{

enum OriginType
{
    inertial,
    from_ephemeris,
    from_integration
};

//! This class acts as a data container for properties of central bodies in a numerical integration
/*!
 *  This class acts as a data container for properties of central bodies in a numerical integration.
 *  It makes a distinction between central bodies that are integrated and those for which the state is taken from ephemeris
 *  The state of the central bodies in an inertial frame can be retrieved from it.
 */
template< typename StateScalarType = double, typename TimeType = double >
class CentralBodyData
{
public:
    //! Constructor, takes the names of the central bodies and the integrated bodies, as well as a list of body objects.
    /*!
     *  Constructor, takes the names of the central bodies and the integrated bodies, as well as a list of body objects.
     *  Checks which central bodies are integrated bodies and sets the update order of the bodies' states accordingly.
     *  \param centralBodies Names of central bodies, belonging to the entries in the bodiesToIntegrate vector of same index.
     *  \param bodiesToIntegrate Names of bodies that are to be integrated numerically.
     *  \param bodyStateFunctions List of functions for the origins of selected bodies.
     */
    CentralBodyData( const std::vector< std::string >& centralBodies,
                     const std::vector< std::string >& bodiesToIntegrate,
                     const std::map< std::string, boost::function< Eigen::Matrix< StateScalarType, 6, 1 >( const TimeType ) > >& bodyStateFunctions ):
        centralBodies_( centralBodies )
    {
        // Check consistency of input.
        if( centralBodies.size( ) != bodiesToIntegrate.size( ) )
        {
            std::cerr<<"Error, number of central bodies not equal to number of bodies to integrate "<<std::endl;
        }


        bodyOriginType_.resize( bodiesToIntegrate.size( ) );

        // Iterate over all integrated bodies to set body origin, its type and associated data.
        for( unsigned int i = 0; i < bodiesToIntegrate.size( ); i++ )
        {
            // Check if central body is inertial.
            if( ( centralBodies.at( i ) == "Inertial" ) || ( centralBodies.at( i ) == "" ) ||( centralBodies.at( i ) == "SSB" ) )
            {
                bodyOriginType_.at( i ) = inertial;
            }
            else
            {
                // Check if central body of current integrated body is also integrated.
                int centralBodyIndex = -1;
                for( unsigned int j = 0; j < bodiesToIntegrate.size( ); j++ )
                {
                    // If there is a match between central body and an integrated body, store body index.
                    if( centralBodies.at( i ) == bodiesToIntegrate[ j ] )
                    {
                        centralBodyIndex = j;
                        if( i == j )
                        {
                            std::cerr<<"Error, body "<< bodiesToIntegrate[ j ]<<" cannot be its own central body"<<std::endl;
                        }
                    }
                }

                // If no integrated central body foundl set body origin as being from an ephemeris.
                if( centralBodyIndex == -1 )
                {
                    bodyOriginType_.at( i ) = from_ephemeris;
                    centralBodiesFromEphemerides_[ i ] = bodyStateFunctions.at( centralBodies.at( i ) );
                }
                // Else, set body origin as being from another integrated body, set indices of both bodies in centralBodiesFromIntegration_
                else
                {
                    bodyOriginType_.at( i ) = from_integration;
                    centralBodiesFromIntegration_[ i ] = centralBodyIndex;
                }
            }
        }

        std::vector< int > numericalBodies_;

        updateOrder_.resize( bodiesToIntegrate.size( ) );
        int currentUpdateIndex = 0;
        for( unsigned int i = 0; i < bodyOriginType_.size( ); i++ )
        {
            // If body origin is not from another integrated body, order in update is irrelevant, set at current index of vector.
            if( bodyOriginType_.at( i ) == inertial || bodyOriginType_.at( i ) == from_ephemeris  )
            {
                updateOrder_[ currentUpdateIndex ] = i;
                currentUpdateIndex++;
            }
            // Else, store index of body having another integrated body as central body
            else if( bodyOriginType_.at( i ) == from_integration )
            {
                numericalBodies_.push_back( i );
            }
        }

        int bodyToMove;
        for( unsigned int i = 0; i < numericalBodies_.size( ); i++ )
        {
            for( unsigned int j = 0; j < i; j++ )
            {
                if( centralBodies[ numericalBodies_[ j ] ] == bodiesToIntegrate[ numericalBodies_.at( i ) ] )
                {
                    // Move central body to index before integrated body.
                    bodyToMove = numericalBodies_.at( i );
                    numericalBodies_.erase( numericalBodies_.begin( ) + i );
                    numericalBodies_.insert( numericalBodies_.begin( ) + j, bodyToMove );
                    break;
                }
            }
        }

        for( unsigned int i = 0; i < numericalBodies_.size( ); i++ )
        {
            updateOrder_[ currentUpdateIndex ] = numericalBodies_.at( i );
            currentUpdateIndex++;
        }
    }


    //! Function to return the state of the central bodies in an inertial frame.
    /*!
     *  Function to return the state of the central bodies in an inertial frame. The states of the integrated bodies in either their local
     *  frame or the inertial frame, and the current time have to be passed as arguments to this function.
     *  \param internalState States of bodies that are numerically integrated, size should be 6 * size of bodiesToIntegrate,
     *  with entries in the order of the bodies in the bodiesToIntegrate vector.
     *  \param time Current time (used for retrieving states from ephemerides)
     *  \param areInputStateLocal True if the internalState vector is given in the local frames of the integrated bodies, or
     *  the global frame.
     *  \return Vector of states of the reference frame origins for each body.
     */
    std::vector<  Eigen::Matrix< StateScalarType, 6, 1 > > getReferenceFrameOriginInertialStates(
            Eigen::Matrix< StateScalarType, Eigen::Dynamic, 1 > internalState, const TimeType time, const bool areInputStateLocal = true )
    {
        std::vector< Eigen::Matrix< StateScalarType, 6, 1 > > referenceFrameOriginStates_;
        referenceFrameOriginStates_.resize( updateOrder_.size( ) );
        for( unsigned int i = 0; i < updateOrder_.size( ); i++ )
        {
            referenceFrameOriginStates_[ updateOrder_[ i ] ] =
                    getSingleReferenceFrameOriginInertialState( internalState, time, updateOrder_[ i ] );
            if( areInputStateLocal )
            {
                internalState.segment( 6 * updateOrder_[ i ], 6 ) += referenceFrameOriginStates_[ updateOrder_[ i ] ];
            }
        }

        return referenceFrameOriginStates_;
    }


    std::vector< int > getUpdateOrder( ){ return updateOrder_; }

    std::vector< OriginType > getBodyOriginType( ){ return bodyOriginType_; }

    std::map< int, int > getCentralBodiesFromIntegration( ){ return centralBodiesFromIntegration_; }

    std::vector< std::string > getCentralBodies( ){ return centralBodies_; }


private:
    std::vector< std::string > centralBodies_;

    std::vector< int > updateOrder_;

    std::vector< OriginType > bodyOriginType_;

    std::map< int, boost::function< Eigen::Matrix< StateScalarType, 6, 1 >( const TimeType ) > > centralBodiesFromEphemerides_;

    std::map< int, int > centralBodiesFromIntegration_;

       Eigen::Matrix< StateScalarType, 6, 1 > getSingleReferenceFrameOriginInertialState(
            const Eigen::Matrix< StateScalarType, Eigen::Dynamic, 1 > internalSolution,
            const TimeType time,
            const int bodyIndex )
    {
        Eigen::Matrix< StateScalarType, 6, 1 > originState = Eigen::Matrix< StateScalarType, 6, 1 >::Zero( );
        switch( bodyOriginType_[ bodyIndex ] )
        {
        case inertial:
            //std::cerr<<"Warning, checking for inertial correction state"<<std::endl;
            break;
        case from_ephemeris:
            originState = centralBodiesFromEphemerides_.at( bodyIndex )( static_cast< double >( time ) );
            break;
        case from_integration:
            //std::cout<<"From int. "<<bodyIndex<<" "<<centralBodiesFromIntegration_.at( bodyIndex )<<std::endl;
            originState = internalSolution.segment( centralBodiesFromIntegration_.at( bodyIndex ) * 6, 6 );
            //std::cout<<"From int. "<<originState.transpose( )<<std::endl;

            break;
        }
        return originState;
    }
};


}

}
#endif // CENTRALBODYDATA_H
