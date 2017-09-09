/*    Copyright (c) 2010-2017, Delft University of Technology
 *    All rigths reserved
 *
 *    This file is part of the Tudat. Redistribution and use in source and
 *    binary forms, with or without modification, are permitted exclusively
 *    under the terms of the Modified BSD license. You should have received
 *    a copy of the license with this file. If not, please or visit:
 *    http://tudat.tudelft.nl/LICENSE.
 */

#ifndef TUDAT_JSONINTERFACE_THRUST_H
#define TUDAT_JSONINTERFACE_THRUST_H

#include <Tudat/SimulationSetup/PropagationSetup/accelerationSettings.h>
#include <Tudat/SimulationSetup/PropagationSetup/thrustSettings.h>

#include "Tudat/External/JsonInterface/Support/valueAccess.h"
#include "Tudat/External/JsonInterface/Support/valueConversions.h"

namespace tudat
{

namespace simulation_setup
{

// ThrustDirectionGuidanceTypes

//! Map of `ThrustDirectionGuidanceTypes` string representations.
static std::map< ThrustDirectionGuidanceTypes, std::string > thrustDirectionTypes =
{
    { colinear_with_state_segment_thrust_direction, "colinearWithStateSegment" },
    { thrust_direction_from_existing_body_orientation, "fromExistingBodyOrientation" },
    { custom_thrust_direction, "customDirection" },
    { custom_thrust_orientation, "customOrientation" }
};

//! `ThrustDirectionGuidanceTypes` not supported by `json_interface`.
static std::vector< ThrustDirectionGuidanceTypes > unsupportedThrustDirectionTypes =
{
    custom_thrust_direction,
    custom_thrust_orientation
};

//! Convert `ThrustDirectionGuidanceTypes` to `json`.
inline void to_json( json& jsonObject, const ThrustDirectionGuidanceTypes& thrustDirectionType )
{
    jsonObject = json_interface::stringFromEnum( thrustDirectionType, thrustDirectionTypes );
}

//! Convert `json` to `ThrustDirectionGuidanceTypes`.
inline void from_json( const json& jsonObject, ThrustDirectionGuidanceTypes& thrustDirectionType )
{
    thrustDirectionType = json_interface::enumFromString( jsonObject, thrustDirectionTypes );
}


// ThrustDirectionGuidanceSettings

//! Create a `json` object from a shared pointer to a `ThrustDirectionGuidanceSettings` object.
void to_json( json& jsonObject, const boost::shared_ptr< ThrustDirectionGuidanceSettings >& directionSettings );

//! Create a shared pointer to a `AccelerationSettings` object from a `json` object.
void from_json( const json& jsonObject, boost::shared_ptr< ThrustDirectionGuidanceSettings >& directionSettings );


// ThrustMagnitudeTypes

//! Map of `ThrustMagnitudeTypes` string representations.
static std::map< ThrustMagnitudeTypes, std::string > thrustMagnitudeTypes =
{
    { constant_thrust_magnitude, "constant" },
    { from_engine_properties_thrust_magnitude, "fromEngineProperties" },
    { thrust_magnitude_from_time_function, "timeDependent" },
    { thrust_magnitude_from_dependent_variables, "variableDependent" }
};

//! `ThrustMagnitudeTypes` not supported by `json_interface`.
static std::vector< ThrustMagnitudeTypes > unsupportedThrustMagnitudeTypes =
{
    thrust_magnitude_from_time_function,
    thrust_magnitude_from_dependent_variables
};

//! Convert `ThrustMagnitudeTypes` to `json`.
inline void to_json( json& jsonObject, const ThrustMagnitudeTypes& thrustMagnitudeType )
{
    jsonObject = json_interface::stringFromEnum( thrustMagnitudeType, thrustMagnitudeTypes );
}

//! Convert `json` to `ThrustMagnitudeTypes`.
inline void from_json( const json& jsonObject, ThrustMagnitudeTypes& thrustMagnitudeType )
{
    thrustMagnitudeType = json_interface::enumFromString( jsonObject, thrustMagnitudeTypes );
}


// ThrustEngineSettings

//! Create a `json` object from a shared pointer to a `ThrustEngineSettings` object.
void to_json( json& jsonObject, const boost::shared_ptr< ThrustEngineSettings >& magnitudeSettings );

//! Create a shared pointer to a `AccelerationSettings` object from a `json` object.
void from_json( const json& jsonObject, boost::shared_ptr< ThrustEngineSettings >& magnitudeSettings );


// ThrustFrames

//! Map of `ThrustFrames` string representations.
static std::map< ThrustFrames, std::string > thrustFrameTypes =
{
    { unspecified_thurst_frame, "unspecified" },
    { inertial_thurst_frame, "intertial" },
    { lvlh_thrust_frame, "lvlh" }
};

//! `ThrustFrames` not supported by `json_interface`.
static std::vector< ThrustFrames > unsupportedThrustFrameTypes = { };

//! Convert `ThrustFrames` to `json`.
inline void to_json( json& jsonObject, const ThrustFrames& thrustFrameType )
{
    jsonObject = json_interface::stringFromEnum( thrustFrameType, thrustFrameTypes );
}

//! Convert `json` to `ThrustFrames`.
inline void from_json( const json& jsonObject, ThrustFrames& thrustFrameType )
{
    thrustFrameType = json_interface::enumFromString( jsonObject, thrustFrameTypes );
}


// Thrust

//! Create a `json` object from a shared pointer to a `ThrustAccelerationSettings` object.
void to_json( json& jsonObject, const boost::shared_ptr< ThrustAccelerationSettings >& thrustAccelerationSettings );

//! Create a shared pointer to a `ThrustAccelerationSettings` object from a `json` object.
void from_json( const json& jsonObject, boost::shared_ptr< ThrustAccelerationSettings >& thrustAccelerationSettings );

} // namespace simulation_setup

} // namespace tudat

#endif // TUDAT_JSONINTERFACE_THRUST_H
