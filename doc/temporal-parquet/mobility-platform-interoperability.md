<!--
Copyright(c) MobilityDB Contributors

This documentation is licensed under a
Creative Commons Attribution-Share Alike 3.0 License
https://creativecommons.org/licenses/by-sa/3.0/
-->

# Mobility data platform interoperability: data and computation

An edge-to-cloud mobility data platform is interoperable only if both the data and the computation move freely across the ecosystem. These are two distinct properties:

- Portable data: mobility values produced by one engine are read, losslessly and without that engine's runtime, by any other.
- Portable computation: the same mobility analytics run, comparably, across the ecosystem's engines.

Each is necessary and neither is sufficient. Portable data without portable computation is a faithful lake nothing can query; portable computation without portable data is engines that cannot read each other's mobility data. Together they are what makes the platform edge-to-cloud.

Each property has a reproducible companion instantiation:

| Property | Companion | Instantiates |
| --- | --- | --- |
| Portable data | [Arrow interchange instantiation](./arrow-interchange-instantiation.md) | the optional Arrow interchange layer of the Temporal Data Lake RFC (#912) |
| Portable computation | [Cross-platform timings](https://github.com/estebanzimanyi/MobilityDB-BerlinMOD/blob/doc/benchmark-restructure/BerlinMOD/benchmarks/CrossPlatform_timings_2026-05-12.md) | the cross-platform mobility-analytics methodology |

The verified data-portability instantiation is the optional Arrow interchange; the ratified-v1 MEOS-WKB and native-scalar sidecar substrate is delivered in the MobilityDuck consumer lane. Each companion is self-contained, independently reproducible, and states its own arguments and measured results.
