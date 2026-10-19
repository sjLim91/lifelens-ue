# Core-issued cognitive context receipts

Related: #689, #790. Depends on the care, Earth persistence and memory attribution changes.

Core request fingerprints now include world seed, population seed and generation version. Browser transport carries decimal strings for all 64-bit identifiers and fingerprints. They are never rounded through JavaScript Number.

A pure observe call does not issue model input. The model transport explicitly issues a bounded receipt (at most 64 outstanding), tied to actor, trigger, world identity, observed minute and fingerprint. Async responses retain the existing 96 simulation-minute window, but require an outstanding Core-issued receipt. Core validates the proposed intent against current legal context, then records both the originally observed context and the current accepted context when they differ. Successful acceptance consumes the receipt. Invalid input, old worlds, unissued observations, duplicate replies and expired receipts fail closed.

Unaccepted receipts are transient transport state. New games and successful restores clear them; loading never re-runs a model. Accepted input persists in the journal. Journal extension v2 is used only for async provenance; v1 remains readable, and synchronous journals retain v1 byte encoding.

Validation: native receipt tests cover cross-world identity, population/generation mismatch, unissued input, async provenance, consumption, expiry, bounded eviction and restore. The live resident AI test uses actual issued bridge contexts. Web regression checks lossless values above 2^53, malformed/incomplete receipts and forwarding the original fingerprint.

This does not prove every accepted preference executes. Physical execution, resource legality and existing Core feedback remain authoritative. Full action-outcome attribution and long-run behavioral assessment remain separate work.
