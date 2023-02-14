/*    Copyright (c) 2010-2023, Delft University of Technology
 *    All rigths reserved
 *
 *    This file is part of the Tudat. Redistribution and use in source and
 *    binary forms, with or without modification, are permitted exclusively
 *    under the terms of the Modified BSD license. You should have received
 *    a copy of the license with this file. If not, please or visit:
 *    http://tudat.tudelft.nl/LICENSE.
 */

#ifndef TUDAT_PARSEODFFILE_H
#define TUDAT_PARSEODFFILE_H

#include "tudat/basics/utilities.h"
#include "tudat/astro/orbit_determination/parseOdfFile.h"
#include "tudat/io/readOdfFile.h"
#include "tudat/astro/observation_models/observableTypes.h"
#include "tudat/math/interpolators/lookupScheme.h"
#include "tudat/math/quadrature/trapezoidQuadrature.h"

namespace tudat
{

namespace orbit_determination
{

observation_models::ObservableType getObservableTypeForOdfId( const int odfId );

std::string getStationNameFromStationId ( const int networkId, const int stationId );

class ProcessedOdfFileSingleLinkData
{
public:

    ProcessedOdfFileSingleLinkData( ){ }

    virtual ~ProcessedOdfFileSingleLinkData( ){ }

    std::vector< double > observationTimes;
    std::vector< double > observableValues;
    std::vector< double > receiverDownlinkDelay;

    std::vector< int > downlinkBand;
    std::vector< int > uplinkBand;
    std::vector< int > referenceBand;

    std::vector< std::string > originFile;

    observation_models::ObservableType observableType;

    std::string transmittingStation;
    std::string receivingStation;
    int transmitterNetworkId;

    std::map< double, double > getObservationData( )
    {
        return utilities::createMapFromVectors( observationTimes, observableValues );
    }
};

class ProcessedOdfFileDopplerData: public ProcessedOdfFileSingleLinkData
{
public:

    ~ProcessedOdfFileDopplerData( ){ }

    std::vector< int > receiverChannels_;
    std::vector< double > referenceFrequencies_;
    std::vector< double > compressionTimes_;
    std::vector< double > uplinkDelays_;
    std::vector< bool > receiverRampingFlags_;

    std::map< double, bool > getReceiverRampingFlags( )
    {
        return utilities::createMapFromVectors( observationTimes, receiverRampingFlags_ );
    }

    std::map< double, double > getReferenceFrequencies( )
    {
        return utilities::createMapFromVectors( observationTimes, referenceFrequencies_ );
    }

    std::map< double, double > getCompressionTimes( )
    {
        return utilities::createMapFromVectors( observationTimes, compressionTimes_ );
    }
};


// TODO: test computation of frequencies and integral
class RampedReferenceFrequencyInterpolator
{
public:
    RampedReferenceFrequencyInterpolator(
            std::vector< std::shared_ptr< input_output::OdfRampBlock > > rampBlock )
    {
        for( unsigned int i = 0; i < rampBlock.size( ); i++ )
        {
            startTimes_.push_back( rampBlock.at( i )->getRampStartTime( ) );
            endTimes_.push_back( rampBlock.at( i )->getRampEndTime( ) );
            rampRates_.push_back( rampBlock.at( i )->getRampRate( ) );
            startFrequencies_.push_back( rampBlock.at( i )->getRampStartFrequency( ) );
        }

        startTimeLookupScheme_ = std::make_shared<
                interpolators::HuntingAlgorithmLookupScheme< double > >(
                startTimes_ );
    }

    RampedReferenceFrequencyInterpolator(
            const std::vector< double >& startTimes_,
            const std::vector< double >& endTimes_,
            const std::vector< double >& rampRates_,
            const std::vector< double >& startFrequency_ ):
            startTimes_( startTimes_ ), endTimes_( endTimes_ ), rampRates_( rampRates_ ), startFrequencies_( startFrequency_ )
    {
        startTimeLookupScheme_ = std::make_shared<
                interpolators::HuntingAlgorithmLookupScheme< double > >(
                startTimes_ );
    }


    double getCurrentReferenceFrequencyIntegral( const double quadratureStartTime, const double quadratureEndTime )
    {
        std::vector< double > quadratureTimes;
        std::vector < double > quadratureFrequencies;

        // Point corresponding to first partial ramp
        quadratureTimes.push_back ( quadratureStartTime );
        quadratureFrequencies.push_back ( getCurrentReferenceFrequency( quadratureStartTime ) );

        // Points corresponding to full ramps
        for( unsigned int i = 1; i < startTimes_.size( ) && startTimes_.at( i ) < quadratureEndTime; i++ )
        {
            quadratureTimes.push_back( startTimes_.at( i ) );
            quadratureFrequencies.push_back( startFrequencies_.at( i ) );
        }

        // Point corresponding to final partial ramp
        quadratureTimes.push_back ( quadratureEndTime );
        quadratureFrequencies.push_back ( getCurrentReferenceFrequency( quadratureEndTime ) );

        return numerical_quadrature::performTrapezoidalQuadrature( quadratureTimes, quadratureFrequencies );
    }

    double getCurrentReferenceFrequency( const double lookupTime )
    {
        int lowerNearestNeighbour = startTimeLookupScheme_->findNearestLowerNeighbour( lookupTime );

        if( lookupTime > endTimes_.at( lowerNearestNeighbour ) || lookupTime < startTimes_.at ( lowerNearestNeighbour ) )
        {
            throw std::runtime_error(
                    "Error when interpolating ODF ramp reference frequency: look up time (" + std::to_string( lookupTime ) +
                    ") is outside the ramp table interval (" + std::to_string( startTimes_.at( 0 ) ) + " to " +
                    std::to_string( startTimes_.back( ) ) + ")." );
        }

        return startFrequencies_.at( lowerNearestNeighbour ) +
               rampRates_.at( lowerNearestNeighbour ) * ( lookupTime - startTimes_.at( lowerNearestNeighbour ) );
    }

    std::vector< double > getStartTimes ( )
    {
        return startTimes_;
    }

    std::vector< double > getEndTimes ( )
    {
        return endTimes_;
    }

    std::vector< double > getRampRates ( )
    {
        return rampRates_;
    }

    std::vector< double > getStartFrequencies ( )
    {
        return startFrequencies_;
    }

private:

    std::vector< double > startTimes_;
    std::vector< double > endTimes_;
    std::vector< double > rampRates_;
    std::vector< double > startFrequencies_;

    std::shared_ptr< interpolators::LookUpScheme< double > > startTimeLookupScheme_;

};


class ProcessedOdfFileContents
{
public:

    std::string spacecraftName_;

    std::map< observation_models::ObservableType, std::map< std::pair< std::string, std::string >,
        std::shared_ptr< ProcessedOdfFileSingleLinkData > > > processedDataBlocks_;

    std::map< int, std::shared_ptr< RampedReferenceFrequencyInterpolator > > rampInterpolators_;
};


std::shared_ptr< RampedReferenceFrequencyInterpolator > mergeRampDataInterpolators(
        const std::vector< std::shared_ptr< RampedReferenceFrequencyInterpolator > >& interpolatorList );

void addOdfFileContentsToMergedContents(
        const observation_models::ObservableType observableType,
        std::shared_ptr< ProcessedOdfFileSingleLinkData > mergedOdfFileContents,
        std::shared_ptr< ProcessedOdfFileSingleLinkData > blockToAdd );

std::shared_ptr< ProcessedOdfFileContents > mergeOdfFileContents(
        const std::vector< std::shared_ptr< ProcessedOdfFileContents > > odfFileContents );

void addOdfDataBlockToProcessedData(
        const observation_models::ObservableType currentObservableType,
        const std::shared_ptr< input_output::OdfDataBlock > rawDataBlock,
        const std::shared_ptr< ProcessedOdfFileSingleLinkData > processedDataBlock );

std::shared_ptr< ProcessedOdfFileContents > processOdfFileContents(
        const std::shared_ptr< input_output::OdfRawFileContents > rawOdfData );

} // namespace orbit_determination

} // namespace tudat

#endif // TUDAT_PARSEODFFILE_H
