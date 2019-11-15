// This file is part of the Acts project.
//
// Copyright (C) 2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Vertexing/TrackAtVertex.hpp"

namespace Acts {

/// @class ImpactPoint3dEstimator
///
/// @brief Estimates point of closest approach in 3D
/// together with corresponding track parameters
template <typename input_track_t, typename propagator_t,
          typename propagator_options_t = PropagatorOptions<>>
class ImpactPoint3dEstimator {
  using BField_t = typename propagator_t::Stepper::BField;

 public:
  /// @struct Configuration struct
  struct Config {
    /// @brief Configuration constructor
    ///
    /// @param bIn The magnetic field
    /// @param prop The propagator
    /// @param propOptions The propagator options
    /// @param doBackwardPropagation Set the propagation direction to backward
    Config(const BField_t& bIn, std::shared_ptr<propagator_t> prop,
           propagator_options_t propOptions, bool doBackwardPropagation = true)
        : bField(bIn),
          propagator(std::move(prop)),
          pOptions(std::move(propOptions)) {
      if (doBackwardPropagation) {
        pOptions.direction = backward;
      }
    }

    /// Magnetic field
    BField_t bField;
    /// Propagator
    std::shared_ptr<propagator_t> propagator;
    // The propagator options
    propagator_options_t pOptions;
    /// Max. number of iterations in Newton method
    int maxIterations = 20;
    /// Desired precision in deltaPhi in Newton method
    double precision = 1.e-10;
  };

  /// @brief Constructor
  ///
  /// @param cfg Configuration object
  ImpactPoint3dEstimator(const Config& cfg) : m_cfg(cfg) {}

  /// @brief Calculates 3D distance between a track and a 3D point
  ///
  /// @param gctx The geometry context
  /// @param trkParams Track parameters
  /// @param vtxPos Position to calculate distance to
  ///
  /// @return Distance
  Result<double> calculateDistance(const GeometryContext& gctx,
                                   const BoundParameters& trkParams,
                                   const Vector3D& vtxPos) const;

  /// @brief Creates track parameters bound to plane
  /// at point of closest approach in 3d to given
  /// reference position. The parameters and errors
  /// are defined on the plane intersecting the track
  /// at point of closest approach, with track orthogonal
  /// to the plane and center of the plane defined as the
  /// given reference point (vertex).
  ///
  /// @param gctx The geometry context
  /// @param trkParams Track parameters
  /// @param vtxPos Reference position (vertex)
  ///
  /// @return New track params
  Result<std::unique_ptr<const BoundParameters>> getParamsAtClosestApproach(
      const GeometryContext& gctx, const BoundParameters& trkParams,
      const Vector3D& vtxPos) const;

  /// @brief Estimates the compatibility of a
  /// track to a vertex position based on the 3d
  /// distance between the track and the vertex
  ///
  /// @param gctx The Geometry context
  /// @param track Track parameters at point of closest
  /// approach in 3d as retrieved by getParamsAtClosestApproach
  /// @param vertexPos The vertex position
  ///
  /// @return The compatibility value
  Result<double> getVertexCompatibility(const GeometryContext& gctx,
                                        const BoundParameters* trkParams,
                                        const Vector3D& vertexPos) const;

 private:
  /// Configuration object
  const Config m_cfg;

  /// @brief Performs a Newton approximation to retrieve a point
  /// of closest approach in 3D to a reference position
  ///
  /// @param trkPos Initial position
  /// @param vtxPos Reference position
  /// @param phi Phi along the helix which will be changed by
  ///        the Newton method
  /// @param theta Track theta
  /// @param r     Helix radius
  ///
  /// @return New phi value
  Result<double> performNewtonApproximation(const Vector3D& trkPos,
                                            const Vector3D& vtxPos, double phi,
                                            double theta, double r) const;

  /// @brief Helper function to calculate relative
  /// distance between track and vtxPos and the
  /// direction of the momentum
  ///
  /// @param gctx The geometry context
  /// @param trkParams Track parameters
  /// @param vtxPos The vertex position
  /// @param deltaR Relative position between
  ///   track and vtxPos, to be determined by method
  /// @param momDir Momentum direction, to be
  ///   determined by method
  Result<void> getDistanceAndMomentum(const GeometryContext& gctx,
                                      const BoundParameters& trkParams,
                                      const Vector3D& vtxPos, Vector3D& deltaR,
                                      Vector3D& momDir) const;

  /// @brief Method that returns the magnetic field value at a given position
  ///        Enabled if BField_t == int (no B-Field provided), returns 0.
  ///
  /// @param linPointPos Position for which to get the magnetic field value
  /// @return The magnetic field value
  template <typename T = BField_t,
            std::enable_if_t<std::is_same<T, int>::value, int> = 0>
  double getBField(const Acts::Vector3D& /*linPointPos*/) const {
    return m_cfg.bField;
  }

  /// @brief Method that returns the magnetic field z-value at a given position
  ///        Enabled if BField_t != int (B-Field provided)
  ///
  /// @param linPointPos Position for which to get the magnetic field value
  /// @return The magnetic field value
  template <typename T = BField_t,
            std::enable_if_t<!std::is_same<T, int>::value, int> = 0>
  double getBField(const Acts::Vector3D& linPointPos) const {
    return m_cfg.bField.getField(linPointPos)[eZ];
  }
};

}  // namespace Acts

#include "Acts/Vertexing/ImpactPoint3dEstimator.ipp"
