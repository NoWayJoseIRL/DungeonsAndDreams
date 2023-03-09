// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "AssetRegistry/AssetData.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWindow.h"

#define LOCTEXT_NAMESPACE "AblAbilityEditor"

class SAblAbilityPropertyDlg : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SAblAbilityPropertyDlg)
	{

	}
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);

	/* Creates and spawns the modal window. Return true if the user hit OK, false otherwise. */
	bool DoModal();

	virtual TSharedRef<SWidget> GetContent() = 0;
	virtual FText GetWindowTitle() const = 0;
	virtual FVector2D GetWindowSize() const { return FVector2D(400, 400); }
protected:
	/* Callback for when the OK button is clicked. */
	FReply OkClicked();

	/* Closes the window. */
	void CloseDialog();

	/* Callback for when the Cancel button is clicked. */
	FReply CancelClicked();

	// Input Override. 
	FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);

	/* A pointer to the window that is asking the user to select an asset.*/
	TWeakPtr<SWindow> m_ModalWindow;

	/* The container */
	TSharedPtr<SVerticalBox> m_Container;

	/* True if Ok was clicked */
	bool m_bOkClicked;

};

template <typename T>
class SAblAbilityNumericPropertyDlg : public SAblAbilityPropertyDlg
{
	SLATE_BEGIN_ARGS(SAblAbilityNumericPropertyDlg)
	{

	}
	SLATE_ARGUMENT(T, DefaultValue);
	SLATE_ARGUMENT(T, MinValue);
	SLATE_ARGUMENT(T, MaxValue);
	SLATE_ARGUMENT(bool, ShowSpinner);
	SLATE_ARGUMENT(FText, PropertyLabel);

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs)
	{
		m_DefaultValue = InArgs._DefaultValue;
		m_MinValue = InArgs._MinValue;
		m_MaxValue = InArgs._MaxValue;
		m_ShowSpinner = InArgs._ShowSpinner;
		m_PropertyLabel = InArgs._PropertyLabel;

		SAblAbilityPropertyDlg::Construct(SAblAbilityPropertyDlg::FArguments());
	}

	T GetValue() const { return m_Value; }

	virtual TSharedRef<SWidget> GetContent() override
	{
		return SNew(SNumericEntryBox<T>)
			.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			.AllowSpin(m_ShowSpinner)
			.MinValue(m_MinValue)
			.MaxValue(m_MaxValue)
			.Value(m_DefaultValue)
			.OnValueChanged(this, &SAblAbilityNumericPropertyDlg::OnValueChanged)
			.OnValueCommitted(this, &SAblAbilityNumericPropertyDlg::OnValueCommitted)
			.Label()
			[
				SNumericEntryBox<T>::BuildLabel(m_PropertyLabel, FLinearColor::White, FLinearColor(0.2f, 0.2f, 0.2f))
			];
	}

	virtual FVector2D GetWindowSize() const { return FVector2D(175, 65); }
	virtual FText GetWindowTitle() const override { return FText::FormatOrdered(LOCTEXT("AblNumericPropertyDlgFmt", "Set {0} value"), m_PropertyLabel); }

	void OnValueCommitted(T NewValue, ETextCommit::Type CommitInfo)
	{
		if (CommitInfo == ETextCommit::OnEnter || CommitInfo == ETextCommit::Default)
		{
			m_Value = NewValue;
			OkClicked();
		}
	}

	void OnValueChanged(T NewValue)
	{
		m_Value = NewValue;
	}

private:
	T m_DefaultValue;
	T m_MinValue;
	T m_MaxValue;
	bool m_ShowSpinner;
	FText m_PropertyLabel;

	T m_Value;
};

class SAblAbilityAssetPropertyDlg : public SAblAbilityPropertyDlg
{
	SLATE_BEGIN_ARGS(SAblAbilityAssetPropertyDlg)
	{

	}
	SLATE_ARGUMENT(FAssetData, DefaultValue);
	SLATE_ARGUMENT(TArray<FTopLevelAssetPath>, AssetFilter);
	SLATE_ARGUMENT(FText, PropertyLabel);

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
	const FAssetData& GetValue() const { return m_Value; }

	virtual TSharedRef<SWidget> GetContent() override;
	virtual FVector2D GetWindowSize() const { return FVector2D(400, 700); }
	virtual FText GetWindowTitle() const override { return FText::FormatOrdered(LOCTEXT("AblAssetPropertyDlgFmt", "Select {0} Asset"), m_PropertyLabel); }

private:
	void OnAssetSelected(const FAssetData& AssetData);
	void OnAssetDoubleClicked(const FAssetData& AssetData);

	FAssetData m_DefaultValue;
	TArray<FTopLevelAssetPath> m_AssetFilter;
	FText m_PropertyLabel;

	FAssetData m_Value;
};

class SAblAbilityEnumPropertyDlg : public SAblAbilityPropertyDlg
{
	SLATE_BEGIN_ARGS(SAblAbilityEnumPropertyDlg)
	{

	}
	SLATE_ARGUMENT(const UEnum*, EnumClass);
	SLATE_ARGUMENT(FText, PropertyLabel);
	SLATE_ARGUMENT(int32, InitialSelectionIndex);
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
	const int32 GetValue() const { return m_Value; }

	virtual TSharedRef<SWidget> GetContent() override;
	virtual FVector2D GetWindowSize() const { return FVector2D(175, 75); }
	virtual FText GetWindowTitle() const override { return FText::FormatOrdered(LOCTEXT("AblEnumPropertyDlgFmt", "Select {0} value"), m_PropertyLabel); }

private:
	void OnSelectionChanged(TSharedPtr<FString> Value, ESelectInfo::Type SelectInfo);

	const UEnum* m_EnumClass;
	FText m_PropertyLabel;
	int32 m_InitialIndex;

	TArray<TSharedPtr<FString>> m_EnumNames;

	int32 m_Value;
};


#undef LOCTEXT_NAMESPACE