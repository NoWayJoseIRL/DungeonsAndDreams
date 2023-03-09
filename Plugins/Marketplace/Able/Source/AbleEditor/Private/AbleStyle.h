// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Templates/SharedPointer.h"
#include "UObject/NameTypes.h"

struct FSlateBrush;

/* This class is our Style Set for Able's Ability Editor. */
class FAbleStyle
{
public:
	/* Initialize the Style Set. */
	static void Initialize();

	/* Shutdown the Style set. */
	static void Shutdown();

	/* Grab the Style Set (singleton). */
	static TSharedPtr< class ISlateStyle > Get();

	/* Returns the name of the Style Set. */
	static FName GetStyleSetName() { return m_StyleName; }

	/* Returns the given Brush according to the passed in BrushName. Returns nullptr if not found. */
	static const FSlateBrush* GetBrush(FName BrushName);
private:
	/* Helper method to grab the Plugin's content folder. */
	static FString InContent(const FString& RelativePath, const ANSICHAR* Extension);

	/* Our Style Set. */
	static TSharedPtr< class FSlateStyleSet > m_StyleSet;
	
	/* The name of our Style Set. */
	static FName m_StyleName;
};