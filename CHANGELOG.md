# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.4.0] - 2026-05-04
@joergbrech, @AntonReiswich: Tagging you here. You might need to include this into TiGL / geoml.

### Changed
 - Gordon surfaces can be created now without the need to duplicate curves in case of closed surfaces.
   This should make it much easier to integrate into CAD tools.
   __Note__: this is optional. The previous behavior still works.
   Thanks: @Shkolik 🙏
 - Improved python package build workflow.

### Fixed
 - Fixed gradient computation in B-spline intersection. This should improve
   robustness and convergence of the intersection computation.
   Thanks: @gongfan99 🙏
 - Ensured scale-independent behaviour. The network interpolation input should work
   with arbitrary scaled input curves.
