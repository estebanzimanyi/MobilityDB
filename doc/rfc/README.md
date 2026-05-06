# MobilityDB RFCs and Design Notes

Two categories of community document are used in MobilityDB. Choosing the right one
depends on whether the question is **ecosystem-wide** or **single-project**:

| | RFC | Design Note |
|---|---|---|
| **Scope** | Ecosystem-level — affects multiple bindings, tools, or upstream projects | Single project or component |
| **Audience** | Open community sign-off | Named reviewers |
| **Discussion category** | **Ideas** | **General** |
| **Branch** | `rfc/<name>` with `doc/rfc/<name>/README.md` | None (wiki page only) |
| **Document structure** | The Problem → Why Now → Proposal → Alternatives Considered → Open Questions → Related | Trade-offs → Decision |
| **Wiki** | [RFCs and Community Proposals](https://github.com/MobilityDB/MobilityDB/wiki/RFCs-and-Community-Proposals) | [Design notes](https://github.com/MobilityDB/MobilityDB/wiki) section on Home |

**The quick test:** if the proposal would require every binding or tool to update in concert, it is an RFC. If it is a decision or plan scoped to one project with named reviewers, it is a Design Note.

---

## Active RFCs

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

1. Author opens a branch `rfc/<name>` and adds `doc/rfc/<name>/README.md` using the canonical structure
2. Author opens a GitHub Discussion (category: **Ideas**) linking to the branch
3. Community comments on the Discussion; the document is revised in-branch
4. Maintainers close the Discussion with an outcome: **accepted** (implementation PR follows),
   **deferred** (valid idea, not now), or **withdrawn**
5. Add a row to this index and to the [wiki RFC table](https://github.com/MobilityDB/MobilityDB/wiki/RFCs-and-Community-Proposals)

Cross-project RFCs (like pcpoint libpc) first gather MobilityDB community context via a
Discussion here, then the author files a PR against the upstream project citing the Discussion.

## Design Note process

1. Author opens a GitHub Discussion (category: **General**) with the plan and named reviewers
2. Author writes a wiki page summarising: context, what changes, trade-offs, open questions, link to Discussion
3. Add the wiki page to the **Design notes** section of [Home](https://github.com/MobilityDB/MobilityDB/wiki)
4. No branch or `doc/rfc/` entry

Active design notes: [tpcbox vs stbox + pcid](https://github.com/MobilityDB/MobilityDB/wiki/Design-note-stbox-vs-tpcbox) · [MobilityAPI backend-agnostic refactor](https://github.com/MobilityDB/MobilityDB/wiki/MobilityAPI-backend-agnostic-refactor) ([Discussion #835](https://github.com/MobilityDB/MobilityDB/discussions/835))
