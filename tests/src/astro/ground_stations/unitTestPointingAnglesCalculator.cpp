/*    Copyright (c) 2010-2019, Delft University of Technology
 *    All rigths reserved
 *
 *    This file is part of the Tudat. Redistribution and use in source and
 *    binary forms, with or without modification, are permitted exclusively
 *    under the terms of the Modified BSD license. You should have received
 *    a copy of the license with this file. If not, please or visit:
 *    http://tudat.tudelft.nl/LICENSE.
 */

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>

#include "tudat/basics/testMacros.h"

#include "tudat/astro/basic_astro/unitConversions.h"
#include "tudat/astro/ground_stations/pointingAnglesCalculator.h"
#include "tudat/astro/ground_stations/groundStationState.h"
#include "tudat/astro/basic_astro/sphericalBodyShapeModel.h"
#include "tudat/math/basic/coordinateConversions.h"
#include "tudat/simulation/environment_setup/body.h"
#include "tudat/simulation/estimation.h"

using namespace tudat::simulation_setup;
using namespace tudat::basic_astrodynamics;
using namespace tudat::ground_stations;
using namespace tudat::observation_models;
using namespace tudat::spice_interface;
using namespace tudat::ephemerides;
using namespace tudat::unit_conversions;
using namespace tudat;


namespace tudat
{
namespace unit_tests
{

BOOST_AUTO_TEST_SUITE( test_pointing_angles_calculator )

// Testing computation of azimuth and elevation given a topocentric vector
// Values compared against data generated by PyGeodesy (https://github.com/mrJean1/PyGeodesy) with the class ltpTuples.Ned
BOOST_AUTO_TEST_CASE( test_TopocentricVectorToAzEl )
{
    Eigen::Vector3d topocentricVector;
    double expectedAzimuth;
    double expectedElevation;

    double degreesToRadians = unit_conversions::convertDegreesToRadians( 1.0 );

    // Define Earth shape model (sphere)
    std::shared_ptr< SphericalBodyShapeModel > bodyShape = std::make_shared< SphericalBodyShapeModel >( 6.371E6 );
    // Create pointing angles calculator
    std::shared_ptr< GroundStationState > stationState = std::make_shared< GroundStationState >(
                Eigen::Vector3d::Zero(), coordinate_conversions::cartesian_position, bodyShape );
    std::shared_ptr< PointingAnglesCalculator > pointAnglesCalculator = std::make_shared< PointingAnglesCalculator >(
                [ & ]( const double ){ return Eigen::Quaterniond( Eigen::Matrix3d::Identity( ) ); },
                std::bind( &GroundStationState::getRotationFromBodyFixedToTopocentricFrame, stationState, std::placeholders::_1 ) );

    // Case 1
    topocentricVector = ( Eigen::Vector3d( ) << 69282032.302755102515, 0, 39999999.999999992549 ).finished( );
    expectedAzimuth = 90.0 * degreesToRadians;
    expectedElevation = 30.0 * degreesToRadians;

    BOOST_CHECK_CLOSE_FRACTION( expectedAzimuth, pointAnglesCalculator->calculateAzimuthAngle( topocentricVector ), 3.0 * std::numeric_limits< double >::epsilon( ) );
    BOOST_CHECK_CLOSE_FRACTION( expectedElevation, pointAnglesCalculator->calculateElevationAngle( topocentricVector ), 3.0 * std::numeric_limits< double >::epsilon( ) );

    // Case 2
    topocentricVector = ( Eigen::Vector3d( ) << 7806858.1854810388759, 74277294.019097447395, 28669435.963624030352 ).finished( );
    expectedAzimuth = 6.0 * degreesToRadians;
    expectedElevation = 21.0 * degreesToRadians;

    BOOST_CHECK_CLOSE_FRACTION( expectedAzimuth, pointAnglesCalculator->calculateAzimuthAngle( topocentricVector ), 3.0 * std::numeric_limits< double >::epsilon( ) );
    BOOST_CHECK_CLOSE_FRACTION( expectedElevation, pointAnglesCalculator->calculateElevationAngle( topocentricVector ), 3.0 * std::numeric_limits< double >::epsilon( ) );

    // Case 1
    topocentricVector = ( Eigen::Vector3d( ) << -37054487.969432726502, -51001127.313443794847, -49252918.02605265379 ).finished( );
    expectedAzimuth = ( 216.0 - 360.0 ) * degreesToRadians;
    expectedElevation = -38.0 * degreesToRadians;

    BOOST_CHECK_CLOSE_FRACTION( expectedAzimuth, pointAnglesCalculator->calculateAzimuthAngle( topocentricVector ), 3.0 * std::numeric_limits< double >::epsilon( ) );
    BOOST_CHECK_CLOSE_FRACTION( expectedElevation, pointAnglesCalculator->calculateElevationAngle( topocentricVector ), 3.0 * std::numeric_limits< double >::epsilon( ) );
}

BOOST_AUTO_TEST_CASE( test_PointingAnglesCalculator )
{
    // Define Earth shape model (sphere)
    std::shared_ptr< SphericalBodyShapeModel > bodyShape = std::make_shared< SphericalBodyShapeModel >( 6.371E6 );

    // Define test ground station point.
    double stationAltitude = 0.0;
    double stationLatitude = convertDegreesToRadians( 20.0 ); //lat;
    double stationLongitude = convertDegreesToRadians( -10.0 ); //lon;

    double degreesToRadians = unit_conversions::convertDegreesToRadians( 1.0 );

    // Test analytically checked azimuth and elevation
    {
        // Create ground station properties
        std::shared_ptr< GroundStationState > stationState = std::make_shared< GroundStationState >(
                    ( Eigen::Vector3d( ) << stationAltitude, stationLatitude, stationLongitude ).finished( ),
                    coordinate_conversions::geodetic_position, bodyShape );
        std::shared_ptr< PointingAnglesCalculator > pointAnglesCalculator = std::make_shared< PointingAnglesCalculator >(
                    [ & ]( const double ){ return Eigen::Quaterniond( Eigen::Matrix3d::Identity( ) ); },
                    std::bind( &GroundStationState::getRotationFromBodyFixedToTopocentricFrame, stationState, std::placeholders::_1 ) );

        // Define state of viewed point
        double testLatitude = 0.0 * degreesToRadians;
        double testLongitude = 0.0 * degreesToRadians;
        double testRadius = 8.0E7;
        Eigen::Vector3d testSphericalPoint( testRadius, mathematical_constants::PI / 2.0 - testLatitude, testLongitude );
        Eigen::Vector3d testCartesianPoint = coordinate_conversions::convertSphericalToCartesian( testSphericalPoint );

        std::cerr << std::setprecision(20) << "testCartesianPoint: " << testCartesianPoint.transpose() << std::endl;

        // Compute azimuth/elevation angles from PointingAnglesCalculator
        double testAzimuth = pointAnglesCalculator->calculateAzimuthAngle( testCartesianPoint, 0.0 );
        double testElevation = pointAnglesCalculator->calculateElevationAngle( testCartesianPoint, 0.0 );

        std::cerr << std::setprecision(20);
        std::cerr << "Az [deg]:" << testAzimuth / degreesToRadians << std::endl;
        std::cerr << "El [deg]:" << testElevation / degreesToRadians << std::endl;

//        double expectedAzimuth = 90.0 * degreesToRadians;
//        double expectedElevation = 60.0 * degreesToRadians;

        BOOST_CHECK_CLOSE_FRACTION( expectedAzimuth, testAzimuth, 3.0 * std::numeric_limits< double >::epsilon( ) );
        BOOST_CHECK_CLOSE_FRACTION( expectedElevation, testElevation, 3.0 * std::numeric_limits< double >::epsilon( ) );
    }

}

BOOST_AUTO_TEST_CASE( test_PointingAnglesCalculator )
{
    // Define Earth shape model (sphere)
    std::shared_ptr< SphericalBodyShapeModel > bodyShape = std::make_shared< SphericalBodyShapeModel >( 6.371E6 );

    // Define test ground station point.
    double groundStationDistance = 6371.0E3;
    Eigen::Vector3d groundStationPosition( groundStationDistance, 0.0, 0.0 );

    double degreesToRadians = unit_conversions::convertDegreesToRadians( 1.0 );

    // Test analytically checked azimuth and elevation
    {
        // Create ground station properties
        std::shared_ptr< GroundStationState > stationState = std::make_shared< GroundStationState >(
                    groundStationPosition, coordinate_conversions::cartesian_position, bodyShape );
        std::shared_ptr< PointingAnglesCalculator > pointAnglesCalculator = std::make_shared< PointingAnglesCalculator >(
                    [ & ]( const double ){ return Eigen::Quaterniond( Eigen::Matrix3d::Identity( ) ); },
                    std::bind( &GroundStationState::getRotationFromBodyFixedToTopocentricFrame, stationState, std::placeholders::_1 ) );

        // Define state of viewed point
        double testLatitude = 30.0 * degreesToRadians;
        double testLongitude = 0.0;
        double testRadius = 8.0E7;
        Eigen::Vector3d testSphericalPoint( testRadius, mathematical_constants::PI / 2.0 - testLatitude, testLongitude );
        Eigen::Vector3d testCartesianPoint = coordinate_conversions::convertSphericalToCartesian( testSphericalPoint );

        std::cerr << std::setprecision(20) << "testCartesianPoint: " << testCartesianPoint.transpose() << std::endl;

        // Compute azimuth/elevation angles from PointingAnglesCalculator
        double testAzimuth = pointAnglesCalculator->calculateAzimuthAngle( testCartesianPoint, 0.0 );
        double testElevation = pointAnglesCalculator->calculateElevationAngle( testCartesianPoint, 0.0 );

        double expectedAzimuth = 90.0 * degreesToRadians;
        double expectedElevation = 60.0 * degreesToRadians;

        BOOST_CHECK_CLOSE_FRACTION( expectedAzimuth, testAzimuth, 3.0 * std::numeric_limits< double >::epsilon( ) );
        BOOST_CHECK_CLOSE_FRACTION( expectedElevation, testElevation, 3.0 * std::numeric_limits< double >::epsilon( ) );
    }

    // Compare results with data obtained from: http://www.movable-type.co.uk/scripts/latlong.html
    {
        {
            // Create ground station properties
            std::shared_ptr< GroundStationState > stationState = std::make_shared< GroundStationState >(
                        groundStationPosition, coordinate_conversions::cartesian_position, bodyShape );
            std::shared_ptr< PointingAnglesCalculator > pointAnglesCalculator = std::make_shared< PointingAnglesCalculator >(
                        [ & ]( const double ){ return Eigen::Quaterniond( Eigen::Matrix3d::Identity( ) ); },
                        std::bind( &GroundStationState::getRotationFromBodyFixedToTopocentricFrame, stationState, std::placeholders::_1 ) );

            // Define state of viewed point
            double testLatitude = 21.0 * degreesToRadians;
            double testLongitude = 84.0 * degreesToRadians;
            double testRadius = 8.0E7;
            Eigen::Vector3d testSphericalPoint;
            testSphericalPoint << testRadius, mathematical_constants::PI / 2.0 - testLatitude, testLongitude;
            Eigen::Vector3d testCartesianPoint = coordinate_conversions::convertSphericalToCartesian( testSphericalPoint );

            std::cerr << "testCartesianPoint: " << testCartesianPoint.transpose() << std::endl;

            // Compute azimuth/elevation angles from PointingAnglesCalculator
            double testAzimuth = pointAnglesCalculator->calculateAzimuthAngle( testCartesianPoint, 0.0 );
            double testElevation = pointAnglesCalculator->calculateElevationAngle( testCartesianPoint, 0.0 );

            // Set azimuth/elevation angles retrieved from website.
            double expectedElevation = mathematical_constants::PI / 2.0 - 9385.0 / 6371.0;
            double expectedAzimuth = mathematical_constants::PI / 2.0 - ( 68.0 + 53.0 / 60.0 + 40.0 / 3600.0 ) * degreesToRadians;

            BOOST_CHECK_CLOSE_FRACTION( expectedAzimuth, testAzimuth, 1.0E-5 );
            BOOST_CHECK_CLOSE_FRACTION( expectedElevation, testElevation, 1.0E-3 );
        }

        {
            // Create ground station properties
            std::shared_ptr< GroundStationState > stationState = std::make_shared< GroundStationState >(
                        groundStationPosition, coordinate_conversions::cartesian_position, bodyShape );
            std::shared_ptr< PointingAnglesCalculator > pointAnglesCalculator = std::make_shared< PointingAnglesCalculator >(
                        [ & ]( const double ){ return Eigen::Quaterniond( Eigen::Matrix3d::Identity( ) ); },
                        std::bind( &GroundStationState::getRotationFromBodyFixedToTopocentricFrame, stationState, std::placeholders::_1 ) );

            // Define state of viewed point
            double testLatitude = -38.0 * degreesToRadians;
            double testLongitude = 234.0 * degreesToRadians;
            double testRadius = 8.0E7;
            Eigen::Vector3d testSphericalPoint;
            testSphericalPoint << testRadius, mathematical_constants::PI / 2.0 - testLatitude, testLongitude;
            Eigen::Vector3d testCartesianPoint = coordinate_conversions::convertSphericalToCartesian( testSphericalPoint );

            std::cerr << "testCartesianPoint: " << testCartesianPoint.transpose() << std::endl;

            // Compute azimuth/elevation angles from PointingAnglesCalculator
            double testAzimuth = pointAnglesCalculator->calculateAzimuthAngle( testCartesianPoint, 0.0 );
            double testElevation = pointAnglesCalculator->calculateElevationAngle( testCartesianPoint, 0.0 );

            // Set azimuth/elevation angles retrieved from website.
            double expectedElevation = mathematical_constants::PI / 2.0 - 13080.0 / 6371.0;
            double expectedAzimuth = mathematical_constants::PI / 2.0 - ( 225.0 + 59.0 / 60.0 + 56.0 / 3600.0 ) * degreesToRadians;

            BOOST_CHECK_CLOSE_FRACTION( expectedAzimuth, testAzimuth, 1.0E-5 );
            BOOST_CHECK_CLOSE_FRACTION( expectedElevation, testElevation, 3.0E-2 );

            std::pair< double, double > pointingAngles = pointAnglesCalculator->calculatePointingAngles( testCartesianPoint, 0.0 );

            BOOST_CHECK_CLOSE_FRACTION( pointingAngles.second, testAzimuth, 1.0E-5 );
            BOOST_CHECK_CLOSE_FRACTION( pointingAngles.first, testElevation, 3.0E-2 );
        }
    }

    // Check if inertial->topocentric rotation is handled consistently
    {
        // Define inertial->body-fixed frame rotation
        double poleRightAscension = 56.0 * degreesToRadians;
        double poleDeclination = 45.0 * degreesToRadians;
        Eigen::Quaterniond inertialToBodyFixedFrame =
                reference_frames::getInertialToPlanetocentricFrameTransformationQuaternion(
                    poleDeclination, poleRightAscension, 0.0 );

        // Define ground station position
        groundStationPosition = Eigen::Vector3d( 1234.0E3, -4539E3, 4298E3 );
        std::shared_ptr< GroundStationState > stationState = std::make_shared< GroundStationState >(
                    groundStationPosition, coordinate_conversions::cartesian_position, bodyShape );
        std::shared_ptr< PointingAnglesCalculator > pointAnglesCalculator = std::make_shared< PointingAnglesCalculator >(
                    [ & ]( const double ){ return inertialToBodyFixedFrame; },
                    std::bind( &GroundStationState::getRotationFromBodyFixedToTopocentricFrame, stationState, std::placeholders::_1 ) );

        // Define state of viewed point
        double testLatitude = -38.0 * degreesToRadians;
        double testLongitude = 234.0 * degreesToRadians;
        double testRadius = 8.0E7;
        Eigen::Vector3d testSphericalPoint = Eigen::Vector3d(
                    testRadius, mathematical_constants::PI / 2.0 - testLatitude, testLongitude );
        Eigen::Vector3d testCartesianPoint = coordinate_conversions::convertSphericalToCartesian( testSphericalPoint );

        // Retrieve topocentric position of viewed point from GroundStationState and PointingAnglesCalculator and compare.
        Eigen::Vector3d testPointInLocalFrame = pointAnglesCalculator->convertVectorFromInertialToTopocentricFrame(
                    testCartesianPoint, 0.0 );
        Eigen::Vector3d expectedTestPointInLocalFrame =
                stationState->getRotationFromBodyFixedToTopocentricFrame( 0.0 ) * inertialToBodyFixedFrame *
                testCartesianPoint ;
        TUDAT_CHECK_MATRIX_CLOSE_FRACTION(
                    testPointInLocalFrame, expectedTestPointInLocalFrame, ( 10.0 * std::numeric_limits< double >::epsilon( ) ) );
    }

}

BOOST_AUTO_TEST_CASE( test_PointingAnglesCalculator2 )
{
        std::cout.precision( 20 );

    // Load ephemerides
//    std::vector< std::string > alternativeKernelsFiles;
//    alternativeKernelsFiles.push_back( getInputPath( ) + "kernels/kernel_system_de440.bsp" );
//    alternativeKernelsFiles.push_back( getInputPath( ) + "kernels/simulation_kernel_juno.bsp" );
    loadStandardSpiceKernels( /*alternativeKernelsFiles*/ );
    loadStandardSpiceKernels( std::vector< std::string >( { "/Users/pipas/Documents/simulation_kernel_juno.bsp" } ) );

    // Simulation epochs
    double initialEpoch = 22.0 * physical_constants::JULIAN_YEAR;
    double finalEpoch = 25.0 * physical_constants::JULIAN_YEAR;

    double j2000Day = 2451544.5;
//    double perijoveTime = ( 2460232.9534491 - j2000Day ) * 86400.0; //PJ1
//    double perijoveTime = ( 2460271.0116551 - j2000Day ) * 86400.0; //PJ2
//    double perijoveTime = ( 2460309.0252315 - j2000Day ) * 86400.0; //PJ3
    double perijoveTime = ( 2460344.4079861 - j2000Day ) * 86400.0; //PJ4
    std::cout << ( perijoveTime - 12.0 * 3600.0 ) / 86400.0 + j2000Day << "\n\n";
    std::cout << ( perijoveTime + 12.0 * 3600.0 ) / 86400.0 + j2000Day << "\n\n";

    double time = ( 2460308.5326389 - j2000Day ) * 86400.0;

    std::string globalFrameOrientation = "J2000";
    std::string globalFrameOrigin = "Earth";


    // Create bodies
    std::vector< std::string > bodiesToCreate = { "Earth", "Jupiter" };
    BodyListSettings bodySettings = getDefaultBodySettings( bodiesToCreate, initialEpoch, finalEpoch, globalFrameOrigin, globalFrameOrientation );

    SystemOfBodies bodies = createSystemOfBodies( bodySettings );


    std::map< double, Eigen::Vector6d > junoStateMap;
    for ( double time = initialEpoch ; time < finalEpoch ; time += 1000.0 )
    {
        junoStateMap[ time ] = spice_interface::getBodyCartesianStateAtEpoch( "-61", "Jupiter", globalFrameOrientation, "None", time );
    }
    std::shared_ptr< interpolators::OneDimensionalInterpolator< double, Eigen::Vector6d > > junoStateInterpolator =
            std::make_shared< interpolators::CubicSplineInterpolator< double, Eigen::Vector6d > >( junoStateMap );

    std::shared_ptr< TabulatedCartesianEphemeris< > > tabulatedEphemeris = std::make_shared< TabulatedCartesianEphemeris< > >(
            junoStateInterpolator, "Jupiter", "J2000");

    bodies.createEmptyBody( "Juno" );
    bodies.at( "Juno" )->setEphemeris( tabulatedEphemeris );




    std::map< std::pair< double, double >, int > visibility;

//    double lonStep = convertDegreesToRadians( 5.0 );
//    double latStep = convertDegreesToRadians( 2.0 );
//    for ( double lon = convertDegreesToRadians( -179.0 ) ; lon < convertDegreesToRadians( 179.0 ) ; lon += lonStep )
//    {
//        for ( double lat = convertDegreesToRadians( -89.0 ) ; lat < convertDegreesToRadians( 89.0 ) ; lat += latStep )
//        {

        // Create ground station
        double stationAltitude = 0.0;
        double stationLatitude = convertDegreesToRadians( 180.0 - 77.0 ); //lat;
        double stationLongitude = convertDegreesToRadians( - 44.0 ); //lon;

        std::pair< std::string, std::string > station = std::pair< std::string, std::string >( "Earth", "Station" );
        createGroundStation( bodies.at( "Earth" ), "Station", ( Eigen::Vector3d( ) << stationAltitude, stationLatitude, stationLongitude ).finished( ),
                             coordinate_conversions::geodetic_position );

        // Retrieve pointing angle calculator
        std::shared_ptr< PointingAnglesCalculator > pointingAngleCalculator =
                bodies.at( "Earth" )->getGroundStation( "Station" )->getPointingAnglesCalculator( );

        std::vector< std::pair< int, int > > linkEndIndices = { { 1, 0 } };

        // Define link end times and states
        std::vector< double > linkEndTimes;
        linkEndTimes.resize( 2 );
        linkEndTimes[ 0 ] = time; //perijoveTime;
        linkEndTimes[ 1 ] = time; //perijoveTime;

        // Get inertial ground station state function
        std::function< Eigen::Vector6d( const double ) > groundStationStateFunction =
                getLinkEndCompleteEphemerisFunction( std::make_pair< std::string, std::string >( "Earth", "Station" ), bodies );

        std::function< Eigen::Vector6d( const double ) > targetFunction =
                getLinkEndCompleteEphemerisFunction( std::make_pair< std::string, std::string >( "Jupiter", "" ), bodies );

        std::vector< Eigen::Vector6d > linkEndStates;
        linkEndStates.resize( 2 );
        linkEndStates[ 1 ] = targetFunction( linkEndTimes[ 0 ] ); // spice_interface::getBodyCartesianStateAtEpoch( "-61", globalFrameOrigin, globalFrameOrientation, "None", linkEndTimes[ 0 ] );
        linkEndStates[ 0 ] = groundStationStateFunction( linkEndTimes[ 1 ] );

        Eigen::VectorXd state = linkEndStates[ 0 ] - linkEndStates[ 1 ];

        // Transform vector to local topocentric frame.
        Eigen::Vector3d vectorInTopoCentricFrame = pointingAngleCalculator->convertVectorFromInertialToTopocentricFrame( state, linkEndTimes[ 0 ] );

        // Calculate and return elevation angle.
        double angle = mathematical_constants::PI / 2.0 - linear_algebra::computeAngleBetweenVectors(
                vectorInTopoCentricFrame, Eigen::Vector3d::UnitZ( ) );
        std::cout << "angle: " << convertRadiansToDegrees( angle ) << "\n\n";

        Eigen::VectorXd gsToScState = linkEndStates[ 1 ] - linkEndStates[ 0 ];

        std::cout << "EL:" << convertRadiansToDegrees(
                pointingAngleCalculator->calculateElevationAngle( gsToScState.segment(0, 3), linkEndTimes[ 0 ] ) ) << std::endl;

        std::cout << "AZ:" << convertRadiansToDegrees(
                pointingAngleCalculator->calculateAzimuthAngle( gsToScState.segment(0, 3), linkEndTimes[ 0 ] ) ) << std::endl;
}

BOOST_AUTO_TEST_SUITE_END( )

}

}
