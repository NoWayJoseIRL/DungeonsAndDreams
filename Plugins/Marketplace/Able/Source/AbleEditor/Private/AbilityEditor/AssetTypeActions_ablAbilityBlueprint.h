// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "AssetTypeActions/AssetTypeActions_Blueprint.h"

/* Asset Action for Ability Blueprint creation. */
class FAssetTypeActions_AblAbilityBlueprint : public FAssetTypeActions_Blueprint
{
public:
	FAssetTypeActions_AblAbilityBlueprint(EAssetTypeCategories::Type AssetCategory);
	~FAssetTypeActions_AblAbilityBlueprint();

	// IAssetTypeActions Implementation
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	virtual uint32 GetCategories() override { return m_AssetCategory; }
	virtual UThumbnailInfo* GetThumbnailInfo(UObject* Asset) const override;
	// End IAssetTypeActions Implementation

	// FAssetTypeActions_Blueprint interface
	virtual UFactory* GetFactoryForBlueprintType(UBlueprint* InBlueprint) const override;

private:
	/** Returns true if the blueprint is data only */
	bool ShouldUseDataOnlyEditor(const UBlueprint* Blueprint) const;

	EAssetTypeCategories::Type m_AssetCategory;
};