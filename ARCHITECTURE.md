# LifeLens runtime architecture

## Stable data core

`FLLResidentData` is the persistent identity/state record. World actors are views of this record, not the source of truth. That keeps population/family history stable when actors stream out, maps change, or Android memory pressure destroys visuals.

## Game-instance subsystems

- `ULLSimulationSubsystem`: WorldSeed, time, residents, relationships, save/load and first social progression.
- `ULLObservationSubsystem`: selected resident only. UI can stay minimal until the user taps a person.

## Actor layer

- `ALLResidentCharacter`: visual embodiment of a resident GUID. It intentionally owns very little persistent state.
- `ULLDecisionComponent`: first Utility-AI scoring layer. Today it chooses an intent from needs/personality; future StateTree tasks can execute the intent.

## Generation invariant

`NewGame(seed)` always constructs exactly two male and two female adults in deterministic order. A save reload never regenerates them. Starting another New Game with another seed produces another cast.

## Social/family path

The persistent schema and relationship pair records are designed for this progression:

`Stranger -> Acquaintance -> Friend -> Dating -> Partner -> Engaged -> Married`

Parent/child GUID arrays plus pregnancy state are already part of resident persistence. Family behavior will be implemented as simulation rules, not hard-coded cinematics.
