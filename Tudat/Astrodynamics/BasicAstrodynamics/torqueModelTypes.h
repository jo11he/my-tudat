/*    Copyright (c) 2010-2017, Delft University of Technology
 *    All rigths reserved
 *
 *    This file is part of the Tudat. Redistribution and use in source and
 *    binary forms, with or without modification, are permitted exclusively
 *    under the terms of the Modified BSD license. You should have received
 *    a copy of the license with this file. If not, please or visit:
 *    http://tudat.tudelft.nl/LICENSE.
 *
 */

#ifndef TUDAT_TORQUEMODELTYPES_H
#define TUDAT_TORQUEMODELTYPES_H

#include "Tudat/Astrodynamics/BasicAstrodynamics/torqueModel.h"

namespace tudat
{

namespace basic_astrodynamics
{

//! List of torques available in simulations
/*!
 *  List of torques available in simulations. Torque models not defined by this
 *  given enum cannot be used for automatic torque model setup.
 */
enum AvailableTorque
{
    underfined_torque = -1,
    second_order_gravitational_torque = 0,
    aerodynamic_torque = 1
};

//! Function to identify the derived class type of a torque model.
/*!
 *  Function to identify the derived class type of a torque model. The type must be defined
 *  in the AvailableTorque enum to be recognized by this function.
 *  \param torqueModel Torque model of which the type is to be identified.
 *  \return Type of the torqueModel, as identified by AvailableTorque enum.
 */
AvailableTorque getTorqueModelType(
        boost::shared_ptr< basic_astrodynamics::TorqueModel > torqueModel );

//! Function to get a string representing a 'named identification' of an torque type
/*!
 * Function to get a string representing a 'named identification' of an torque type
 * \param torqueType Type of torque model.
 * \return String with torque id.
 */
std::string getTorqueModelName( const AvailableTorque torqueType );

std::vector< boost::shared_ptr< TorqueModel > > getTorqueModelsOfType(
        const std::vector< boost::shared_ptr< TorqueModel > >& fullList,
        const AvailableTorque modelType );
}

}


#endif // TUDAT_TORQUEMODELTYPES_H
