# Advanced example of character movement replication in UE4
Example of how to create a custom repicated character movement with move compression, additional movement flags and client-side prediction.

## Key Features
WallRunning movement and state is implied from character and environment and already replicated variables such as velocity and acceleration.

Additonal controls assosiated with WallRunning are replicated to server via a custom saved moves and move containers. We could utilise custom movement flags for this task, but we opted for expanding the replication system and [add WallRunning movement](https://github.com/Pavel-Konarik/ReplicatedMovementWallrun/blob/main/ShooterMovementReplication.cpp#L196-L220) flags which are replicated to the server.

### Move compression
We combine as many moves as possible to make the code more performant and to save bandwidth. Optimisations in this area include thresholding wall normal delta, rolling back client to average normals, client/server indipendent timers and more. 

## Debugging
We implemented a number of console commands which help with debugging any movement related issues and help explaining the movement process. 
```
int32 CVar_WallRun_ShowAll = 0;
static FAutoConsoleVariableRef CVarWallRunShowForces(TEXT("WallRun.ShowAll"), CVar_WallRun_ShowAll,
	TEXT("Show all forces and events during WallRun movement"), ECVF_Default);

int32 CVar_WallRun_ShowJumps = 0;
static FAutoConsoleVariableRef CVarWallRunShowJumps(TEXT("WallRun.ShowJumps"), CVar_WallRun_ShowJumps,
	TEXT("Show [red] direction of WallRun jumps, [green] last touched point, [blue] character capsule"), ECVF_Default);

...

int32 CVar_WallRun_ShowAcceleration = 0;
static FAutoConsoleVariableRef CVarWallRunWallAcceleration(TEXT("WallRun.ShowAcceleration"), CVar_WallRun_ShowAcceleration,
	TEXT("Show direction of acceleration applied to this character. It is expected to be parallel with the wall if character is moving forward"), ECVF_Default);

int32 CVar_WallRun_ShowState = 0;
static FAutoConsoleVariableRef CVarWallRunWallState(TEXT("WallRun.ShowState"), CVar_WallRun_ShowState,
	TEXT("Shows character capsule coloured differently for each state. Start [green], Mid [yellow], End [red]"), ECVF_Default);
 ```
