// Copyright (c) Extra Life Studios, LLC. All rights reserved.
#pragma once

// One off validation helpers.
class UAbleValidation
{
public:
    static bool CheckDependencies(const TArray<const class UAblAbilityTask *>& dependencies);
};
