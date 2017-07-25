/*    Copyright (c) 2010-2017, Delft University of Technology
 *    All rigths reserved
 *
 *    This file is part of the Tudat. Redistribution and use in source and
 *    binary forms, with or without modification, are permitted exclusively
 *    under the terms of the Modified BSD license. You should have received
 *    a copy of the license with this file. If not, please or visit:
 *    http://tudat.tudelft.nl/LICENSE.
 */

#ifndef TUDAT_JSONINTERFACE_SIMULATION_H
#define TUDAT_JSONINTERFACE_SIMULATION_H

#include <boost/filesystem.hpp>

#include <Tudat/SimulationSetup/tudatSimulationHeader.h>

#include "jsonInterface.h"

#include "Environment/body.h"
#include "Propagation/acceleration.h"
// #include "Propagation/massRate.h"
// #include "Propagation/torque.h"
#include "Propagation/propagator.h"
#include "Mathematics/integrator.h"

namespace tudat
{

namespace json_interface
{

typedef boost::filesystem::path path;


//! -DOC
template< typename TimeType = double, typename StateScalarType = double >
class Simulation
{
public:
    //! Vector with filenames of the Spice kernels located in "Tudat/External/SpiceInterface/Kernels/"
    //! to be used for the propagation.
    std::vector< path > spiceKernels;

    //! Whether to preload the ephemeris of the celestial bodies for the simulation period,
    //! or to retrieve this directly from Spice during the propagation at each integration step.
    //! Preloading Spice data generally results in faster propagations, unless:
    //! * The simulation ends much earlier than the specified maximum simulation end epoch.
    //! * The integrator step-size is very large (in the order of several hours or days).
    bool preloadSpiceData;

    //! Offset for the
    std::pair< TimeType, TimeType > spiceIntervalOffsets;

    //! Initial epoch for the simulation.
    TimeType startEpoch;

    //! Maximum end epoch for the simulation.
    TimeType endEpoch;

    //! Global frame origin.
    std::string globalFrameOrigin;

    //! Global frame orientation.
    std::string globalFrameOrientation;

    //! Vector with the names of all the bodies.
    std::vector< std::string > bodies;

    //! Vector with the names of the celestial bodies (handled by Spice).
    std::vector< std::string > celestialBodies;

    //! Vector with the names of the bodies to be propagated.
    std::vector< std::string > bodiesToPropagate;

    //! Body settings.
    std::map< std::string, boost::shared_ptr< simulation_setup::BodySettings > > bodySettingsMap;

    //! Body map.
    simulation_setup::NamedBodyMap bodyMap;


    ////// Integrated state settings / models

    //! Acceleration settings map.
    BodyToBodyMap< simulation_setup::AccelerationSettings > accelerationSettingsMap;

    //! Aceeleration models map.
    BodyToBodyMap< basic_astrodynamics::AccelerationModel3d > accelerationModelsMap;

    //! Mass rate settings map.
    BodyToBodyMap< simulation_setup::MassRateModelSettings > massRateSettingsMap;

    //! Mass rate models map.
    BodyToBodyMap< basic_astrodynamics::MassRateModel > massRateModelsMap;

    //! Torque settings map.
    BodyToBodyMap< simulation_setup::TorqueSettings > torqueSettingsMap;

    //! Torque models map.
    BodyToBodyMap< basic_astrodynamics::TorqueModel > torqueModelsMap;


    //! Propagation settings.
    boost::shared_ptr< propagators::PropagatorSettings< StateScalarType > > propagationSettings;

    //! Integrator settings.
    boost::shared_ptr< numerical_integrators::IntegratorSettings< TimeType > > integratorSettings;

    //! Dynamics simulator.
    boost::shared_ptr< propagators::DynamicsSimulator< StateScalarType, TimeType > > dynamicsSimulator;

    //! -DOC
    Simulation( const std::string& inputFile )
    {
        setInputFile( inputFile );
        reset( );
    }


    //! -DOC
    void setInputFile( const std::string& inputFile )
    {
        inputFilePath = pathForJSONFile( inputFile );
        settings = parseModularJSONFile( inputFilePath );
    }


    //! Update/create all the objects from the JSON data before the simulation can be run.
    void reset( )
    {
        resetGeneral( );
        resetSpice( );
        resetBodies( );
        resetIntegratedStateModels( );
        resetPropagators( );
        resetIntegrator();
        resetOutput( );
    }


    //! -DOC
    void resetGeneral( )
    {
        // Start and end epochs
        startEpoch = getEpoch< TimeType >( settings, KeyTrees::Simulation::startEpoch );
        endEpoch = getEpoch< TimeType >( settings, KeyTrees::Simulation::endEpoch );

        // Global frame origin and orientation
        globalFrameOrigin = getValue< std::string >( settings, KeyTrees::Simulation::globalFrameOrigin );
        globalFrameOrientation = getValue< std::string >( settings, KeyTrees::Simulation::globalFrameOrientation );
    }


    //! -DOC
    void resetSpice( )
    {
        spiceKernels = getValue< std::vector< path > >( settings, KeyTrees::Simulation::spiceKernels, { } );
        preloadSpiceData = getValue< bool >( settings, KeyTrees::Simulation::preloadSpiceData, true );
        spiceIntervalOffsets = preloadSpiceData ?
                    std::make_pair( -300.0, 300.0 ) : std::make_pair( TUDAT_NAN, TUDAT_NAN );

        // Clear all loaded kernels
        spice_interface::clearSpiceKernels( );

        // Load requested Spice kernels
        for ( path kernelFilePath : spiceKernels )
        {
            spice_interface::loadSpiceKernelInTudat( kernelFilePath.string( ) );
        }
    }


    //! -DOC
    void resetBodies( )
    {
        using namespace simulation_setup;

        std::map< std::string, json > bodySettingsJSON =
                getValue< std::map< std::string, json > >( settings, Keys::bodies );

        std::vector< std::string > defaultBodyNames;
        for ( auto entry : bodySettingsJSON )
        {
            const std::string bodyName = entry.first;
            if ( getValue( settings, { Keys::bodies, bodyName, Keys::Body::useDefaultSettings }, false ) )
            {
                defaultBodyNames.push_back( bodyName );
            }
        }

        // Create map with default body settings.
        bodySettingsMap = getDefaultBodySettings( defaultBodyNames,
                                                  startEpoch - spiceIntervalOffsets.first,
                                                  endEpoch + spiceIntervalOffsets.second );

        // Get body settings from JSON.
        for ( auto entry : bodySettingsJSON )
        {
            const std::string bodyName = entry.first;
            if ( bodySettingsMap.count( bodyName ) )
            {
                // Reset ephemeris and rotational models frames.
                boost::shared_ptr< BodySettings > bodySettings = bodySettingsMap[ bodyName ];
                bodySettings->ephemerisSettings->resetFrameOrientation( globalFrameOrientation );
                bodySettings->rotationModelSettings->resetOriginalFrame( globalFrameOrientation );
                // Update body settings from JSON.
                updateBodySettings( bodySettings, settings, { Keys::bodies, bodyName } );
            }
            else
            {
                // Create body settings from JSON.
                bodySettingsMap[ bodyName ] = createBodySettings( settings, { Keys::bodies, bodyName } );
            }
        }

        // Create bodies.
        bodyMap = createBodies( bodySettingsMap );

        // Finalize body creation.
        setGlobalFrameBodyEphemerides( bodyMap, globalFrameOrigin, globalFrameOrientation );


        // Determine propagated and central bodies


    }

    //! -DOC
    std::map< propagators::IntegratedStateType, std::vector< json > > getMapOfSingleArcPropagatorSettings(
            const KeyTree& keyTree = Keys::propagation )
    {
        using namespace propagators;
        using Keys = Keys::Propagator;
        std::map< IntegratedStateType, std::vector< json > > mapOfSingleArcPropagatorSettings;

        try
        {
            // SingleArc
            const IntegratedStateType integratedStateType = getValue< IntegratedStateType >(
                        settings, keyTree + Keys::integratedStateType );
            if ( integratedStateType == hybrid )
            {
                // MultiType
                const std::map< std::string, json > typesMap =
                        getValue< std::map< std::string, json > >( settings, keyTree + Keys::propagators );
                for ( auto entry : typesMap )
                {
                    const IntegratedStateType subtype = enumFromString( entry.first, integratedStateTypes );
                    const json jsonType = entry.second;
                    mapOfSingleArcPropagatorSettings[ subtype ].push_back( jsonType );
                }
            }
            else
            {
                // SingleType
                mapOfSingleArcPropagatorSettings =
                { { integratedStateType, { getValue< json >( settings, keyTree ) } } };
            }
        }
        catch ( ... )
        {
            // MultiArc
            const std::vector< json > jsonVector = getValue< std::vector< json > >( settings, keyTree );
            for ( unsigned int i = 0; i < jsonVector.size( ); ++i )
            {
                const std::map< IntegratedStateType, std::vector< json > > arcMap =
                        getMapOfSingleArcPropagatorSettings( keyTree + i );
                for ( auto entry : arcMap )
                {
                    const IntegratedStateType integratedStateType = entry.first;
                    const std::vector< json > jsonArcs = entry.second;
                    mapOfSingleArcPropagatorSettings[ integratedStateType ].push_back( jsonArcs );
                }
            }
        }

        return mapOfSingleArcPropagatorSettings;
    }

    //! -DOC
    std::pair< std::vector< std::string >, std::vector< std::string > > getCentralAndPropagatedBodies(
            const propagators::IntegratedStateType integratedStateType )
    {
        std::vector< std::string > centralBodies;
        std::vector< std::string > bodiesToPropagate;

        std::vector< json > jsonVector = getMapOfSingleArcPropagatorSettings( ).at( integratedStateType );
        for ( auto jsonObject : jsonVector )
        {
            for ( auto bodyName : jsonObject[ Keys::Propagator::centralBodies ] )
            {
                centralBodies.push_back( bodyName );
                // FIXME: added multiple times? Order?
            }
            for ( auto bodyName : jsonObject[ Keys::Propagator::bodiesToPropagate ] )
            {
                bodiesToPropagate.push_back( bodyName );
                // FIXME: added multiple times? Order?
            }
        }

        return std::make_pair( centralBodies, bodiesToPropagate );
    }


    //! -DOC
    void resetIntegratedStateModels( )
    {
        using namespace propagators;
        using namespace simulation_setup;

        // Translational
        accelerationSettingsMap =
                getBodyToBodyMap< AccelerationSettings >( settings, Keys::accelerations, createAccelerationSettings );
        std::pair< std::vector< std::string >, std::vector< std::string > > centralAndPropagatedBodies =
                getCentralAndPropagatedBodies( translational_state );
        accelerationModelsMap = createAccelerationModelsMap( bodyMap, accelerationSettingsMap,
                                                             centralAndPropagatedBodies.second,
                                                             centralAndPropagatedBodies.first );

        /* FIXME
        // Mass rate
        massRateSettingsMap = getValue< SelectedMassRateModelMap >( settings, Keys::massRates, { } );
        massRateModelsMap = createMassRateModelsMap( bodyMap, massRateSettingsMap, accelerationModelsMap );

        // Torque
        torqueSettingsMap = getValue< SelectedTorqueMap >( settings, Keys::torques, { } );
        torqueModelsMap = createTorqueModelsMap( bodyMap, torqueSettingsMap );
        */
    }


    //! -DOC
    void resetPropagators( )
    {
        // propagationSettings = createPropagationSettings( settings, Keys::propagation, bodyMap );
    }


    //! -DOC
    void resetIntegrator( )
    {
        // Integrator settings
        integratorSettings = createIntegratorSettings< TimeType >( settings, Keys::integrator, startEpoch );
    }


    //! -DOC
    void resetOutput( )
    {

    }


    //! -DOC
    void run( )
    {
        using namespace propagators;

        // Create simulation object
        dynamicsSimulator = boost::make_shared< SingleArcDynamicsSimulator< StateScalarType, TimeType > >(
                    bodyMap, integratorSettings, propagationSettings, false );

        // FIXME: MultiArc

        boost::shared_ptr< SingleArcDynamicsSimulator< StateScalarType, TimeType > > singleArcDynamicsSimulator
                = boost::dynamic_pointer_cast< SingleArcDynamicsSimulator< StateScalarType, TimeType > >(
                    dynamicsSimulator );
        if ( singleArcDynamicsSimulator )
        {
            singleArcDynamicsSimulator->integrateEquationsOfMotion( propagationSettings->getInitialStates( ) );
            return;
        }

        boost::shared_ptr< MultiArcDynamicsSimulator< StateScalarType, TimeType > > multiArcDynamicsSimulator
                = boost::dynamic_pointer_cast< MultiArcDynamicsSimulator< StateScalarType, TimeType > >(
                    dynamicsSimulator );
        if ( multiArcDynamicsSimulator )
        {
            throw std::runtime_error( "MultiArcDynamicsSimulator not supported by JSON interface." );
        }
    }


    //! -DOC
    void exportResults( )
    {

    }


    //! -DOC
    json getOriginalSettings( )
    {
        return settings;
    }


protected:



private:
    //! Absolute path to the input file.
    path inputFilePath;

    //! JSON object with all the settings from the input file.
    json settings;


};


//! Function to create a `json` object from a `Simulation` object.
//! Called automatically by `nlohmann::json` when using a constructor such as `json( simulation )`.
template< typename TimeType = double, typename StateScalarType = double >
void to_json( json& jsonObject, const Simulation< TimeType, StateScalarType >& simulation )
{
    // Simulation (general settings)
    json jsonSimulation;
    jsonSimulation[ Keys::Simulation::startEpoch ] = simulation.startEpoch;
    jsonSimulation[ Keys::Simulation::endEpoch ] = simulation.endEpoch;
    jsonSimulation[ Keys::Simulation::globalFrameOrigin ] = simulation.globalFrameOrigin;
    jsonSimulation[ Keys::Simulation::globalFrameOrientation ] = simulation.globalFrameOrientation;
    jsonSimulation[ Keys::Simulation::spiceKernels ] = simulation.spiceKernels;
    jsonSimulation[ Keys::Simulation::preloadSpiceData ] = simulation.preloadSpiceData;
    jsonObject[ Keys::simulation ] = jsonSimulation;

    // Bodies
    jsonObject[ Keys::bodies ] = simulation.bodySettingsMap;

    // Accelerations
    jsonObject[ Keys::accelerations ] = simulation.accelerationSettingsMap;

    // Integrator
    jsonObject[ Keys::integrator ] = simulation.integratorSettings;
}


} // namespace json_interface

} // namespace tudat

#endif // TUDAT_JSONINTERFACE_SIMULATION_H
