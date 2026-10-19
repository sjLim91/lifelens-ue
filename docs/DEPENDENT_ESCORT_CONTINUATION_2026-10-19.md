# Dependent sanitation escort continuation

Related #653. Depends on #802.

A dependent's physical escort used to exist only inside the current parenting action. Critical food/water work, predator escape or a failed route could clear that action and lose the obligation to bring the child home.

A dedicated Core runtime state now retains child, pickup origin, site, start minute and return phase. It survives ordinary action cancellation and persists in an optional versioned binary extension; worlds without escorts retain the previous encoding. Old in-flight parenting return payloads upgrade on load when caregiver and child are actually co-located, with deterministic single-carrier selection.

Once interrupted, outbound care becomes a physical return after urgent survival work finishes. The child follows the caregiver's actual navigation steps during survival work and escape. Invalidated sites return without inventing site use. Unreachable routes retain the obligation and retry from the real position. Migration cannot start for a carrier. Other parenting completions cannot steal an actively escorted child.

A return ACK completes transport only, never a second ToiletAssist. The dedicated state clears after actual home arrival. Dead children release the obligation; a dead caregiver leaves the child at its last actual position for existing kin/household/community care. There is no teleport or fabricated successful care.

Validation: critical outbound interruption, actual stepwise return after save/load, interrupted return, site invalidation, normal once-only site use, return ACK duplicate rejection, legacy upgrade, duplicate-carrier rejection, truncated atomic decode, caregiver and child death. Existing competing-parenting regression also runs.

This does not introduce remote custody transfer, a new social handoff policy, or guarantees that a blocked or dying household survives.
