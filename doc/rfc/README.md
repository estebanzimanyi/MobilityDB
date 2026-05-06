# MobilityDB RFCs

Request for Comments documents for MobilityDB ecosystem proposals.
Each RFC has a GitHub Discussion for community sign-off and a branch in this repository
containing the proposal document.

| RFC | Topic | Discussion | Branch | Status |
|-----|-------|-----------|--------|--------|
| npoint portability | Stable network location identifiers across OSM/pgRouting versions | [#863](https://github.com/MobilityDB/MobilityDB/discussions/863) | `rfc/npoint-portability` | Open |
| TemporalParquet | Parquet footer convention for MobilityDB temporal types | [#870](https://github.com/MobilityDB/MobilityDB/discussions/870) · Issue [#830](https://github.com/MobilityDB/MobilityDB/issues/830) · PR [#831](https://github.com/MobilityDB/MobilityDB/pull/831) | `rfc/temporal-parquet` | Open |
| MEOS-API | Versioned JSON catalog for MEOS's C-library public API | [#836](https://github.com/MobilityDB/MobilityDB/issues/836) · PR [#845](https://github.com/MobilityDB/MobilityDB/pull/845) | `rfc/meos-api` | Open |
| pcpoint libpc exposure | Expose serialize/deserialize in `libpc.a` for non-PG consumers | [#869](https://github.com/MobilityDB/MobilityDB/discussions/869) | `rfc/pcpoint-libpc` | Draft |

## RFC documents

- [`doc/rfc/npoint-portability/`](npoint-portability/) → [`doc/npoint-portability/README.md`](../npoint-portability/README.md)
- [`doc/rfc/temporal-parquet/`](temporal-parquet/) → [`doc/temporal-parquet/README.md`](../temporal-parquet/README.md)
- [`doc/rfc/meos-api/`](meos-api/) → [`doc/meos-api/README.md`](../meos-api/README.md)
- [`doc/rfc/pcpoint-libpc/README.md`](pcpoint-libpc/README.md)

## RFC process

1. Author opens a branch `rfc/<name>` and adds `doc/rfc/<name>/README.md`
2. Author opens a GitHub Discussion linking to the branch
3. Community comments on the Discussion; the document is revised
4. Maintainers close the Discussion with an outcome: **accepted** (implementation PR follows),
   **deferred** (valid idea, not now), or **withdrawn**

Cross-project RFCs (like pcpoint libpc) first gather MobilityDB community context via a
Discussion here, then the author files a PR against the upstream project citing the Discussion.
