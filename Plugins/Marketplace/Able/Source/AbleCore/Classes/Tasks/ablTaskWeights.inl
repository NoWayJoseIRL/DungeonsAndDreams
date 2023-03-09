// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#if WITH_EDITOR

// Helpful defines for estimating a task cost, see comments for GetEstimatedTaskCost()
// You can chain these together for your estimate (e.g TASK_EST_SYNC + TASK_EST_TICK, etc)
// The editor will clamp this value to 0.0 - 1.0, so don't worry about managing that yourself.
#define ABLTASK_EST_SYNC 0.1f
#define ABLTASK_EST_ASYNC 0.05f
#define ABLTASK_EST_TICK 0.1f
#define ABLTASK_EST_BP_EVENT 0.1f
#define ABLTASK_EST_COLLISION_SIMPLE_QUERY 0.1f
#define ABLTASK_EST_COLLISION_COMPLEX_QUERY 0.2f
#define ABLTASK_EST_COLLISION_QUERY_RAYCAST 0.25f
#define ABLTASK_EST_SPAWN_ACTOR 0.3f

#endif