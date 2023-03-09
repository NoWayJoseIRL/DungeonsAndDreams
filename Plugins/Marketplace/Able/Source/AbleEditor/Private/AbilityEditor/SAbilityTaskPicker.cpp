// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbilityEditor/SAbilityTaskPicker.h"

#include "AbleEditorPrivate.h"

#include "ablAbility.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Preferences/UnrealEdOptions.h"
#include "Styling/AppStyle.h"
#include "Tasks/IAblAbilityTask.h"
#include "Tasks/ablCustomTask.h"

#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"


#define LOCTEXT_NAMESPACE "AblAbilityEditor"

namespace TreeBuilderHelper
{
	void RecursiveTreeSort(TSharedPtr<FAblAbilityTaskTreeNode>& Node)
	{
		if (Node.IsValid())
		{
			for (TSharedPtr<FAblAbilityTaskTreeNode>& Child : Node->Children)
			{
				RecursiveTreeSort(Child);
			}

			Node->Children.Sort([](const TSharedPtr<FAblAbilityTaskTreeNode>& A, const TSharedPtr<FAblAbilityTaskTreeNode>& B)
			{
				return A->GetText().CompareTo(B->GetText()) < 0;
			});
		}
	}

	TSharedPtr<FAblAbilityTaskTreeNode> FindNodeInArray(const TArray<TSharedPtr<FAblAbilityTaskTreeNode>>& InArray, const FString& Category)
	{
		for (TSharedPtr<FAblAbilityTaskTreeNode> Node : InArray)
		{
			if (Node.IsValid() && Node->CategoryName.ToString().Equals(Category))
			{
				return Node;
			}
		}

		return nullptr;
	}
}

const FName SAblAbilityTaskPicker::Column_TaskName(TEXT("TaskName"));

const FText FAblAbilityTaskTreeNode::GetText() const
{
	if (IsLeaf())
	{
		return Task.GetDefaultObject()->GetTaskName();
	}

	return CategoryName;
}

void SAblAbilityTaskPicker::Construct(const FArguments& InArgs)
{
	m_bOkClicked = false;
	m_TaskClass = UAblAbilityTask::StaticClass();

	ChildSlot
		[
			SNew(SBorder)
			.Visibility(EVisibility::Visible)
			.BorderImage(FAppStyle::GetBrush("Menu.Background"))
			[
				SNew(SBox)
				.Visibility(EVisibility::Visible)
				.WidthOverride(500.0f)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(2.0f, 2.0f)
					[
						SNew(STextBlock)
						.Font(FAppStyle::GetFontStyle("LargeText"))
						.Text(LOCTEXT("TaskClass", "Select a Task"))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(2.0f, 2.0f)
					[
						SAssignNew(m_SearchFilter, SSearchBox)
						.SelectAllTextWhenFocused(true)
						.OnTextChanged(this, &SAblAbilityTaskPicker::OnFilterStringChanged)
					]

					+ SVerticalBox::Slot()
					.FillHeight(1)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
						.Content()
						[
							SNew(SScrollBox)
							+ SScrollBox::Slot()
							[
								SAssignNew(m_TaskClassContainer, SVerticalBox)
							]
						]
					]

				  + SVerticalBox::Slot()
					.VAlign(VAlign_Bottom)
					.HAlign(HAlign_Fill)
					.AutoHeight()
					.Padding(2.0f, 2.0f)
					[
						SNew(SBorder)
						.BorderBackgroundColor(FLinearColor::Gray)
						  [
							  SNew(STextBlock)
							  .Margin(FMargin(2.0f, 2.0f))
							  .AutoWrapText(true)
							  .Font(FAppStyle::GetFontStyle("TinyText"))
							  .Text(this, &SAblAbilityTaskPicker::GetTaskDescription)
						  ]
					]

					// Ok/Cancel buttons
					+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Right)
						.VAlign(VAlign_Bottom)
						.Padding(8)
						[
							SNew(SUniformGridPanel)
							.SlotPadding(FAppStyle::GetMargin("StandardDialog.SlotPadding"))
							.MinDesiredSlotWidth(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
							.MinDesiredSlotHeight(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
							+ SUniformGridPanel::Slot(0, 0)
							[
								SNew(SButton)
								.HAlign(HAlign_Center)
								.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
								.OnClicked(this, &SAblAbilityTaskPicker::OkClicked)
								.Text(LOCTEXT("AblAbilityTaskPickerOK", "OK"))
							]
						+ SUniformGridPanel::Slot(1, 0)
							[
								SNew(SButton)
								.HAlign(HAlign_Center)
								.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
								.OnClicked(this, &SAblAbilityTaskPicker::CancelClicked)
								.Text(LOCTEXT("AblAbilityTaskPickerCancel", "Cancel"))
							]
						]
					]
				]
			];

	BuildTreeData();

	MakeTaskClassPicker();
}

bool SAblAbilityTaskPicker::DoModal()
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("CreateAblAbilityTaskPicker", "Add Ability Task"))
		.ClientSize(FVector2D(400, 700))
		.SupportsMinimize(false).SupportsMaximize(false)
		[
			AsShared()
		];

	m_PickerWindow = Window;

	GEditor->EditorAddModalWindow(Window);

	return m_bOkClicked;
}

void SAblAbilityTaskPicker::OnFilterStringChanged(const FText& InFilterString)
{
	m_NameFilter = InFilterString.ToString();

	// Rebuild our data with the new filter.
	BuildTreeData();

	if (m_TaskTree.IsValid())
	{
		// If we have a filter, auto expand any entries that passed the filter.
		if (!m_NameFilter.IsEmpty())
		{
			for (TSharedPtr<FAblAbilityTaskTreeNode>& TreeNode : m_TreeNodes)
			{
				if (TreeNode.IsValid() && !TreeNode->IsLeaf())
				{
					m_TaskTree->SetItemExpansion(TreeNode, true);
				}
			}
		}

		m_TaskTree->RequestTreeRefresh();
	}
}

TSharedRef<ITableRow> SAblAbilityTaskPicker::OnGenerateRow(TSharedPtr<FAblAbilityTaskTreeNode> Entry, const TSharedRef<STableViewBase>& Table)
{
	return SNew(SAblAbilityTaskTreeNodeWidget, Table).TreeNode(Entry).TreeWidget(SharedThis(this));
}

void SAblAbilityTaskPicker::OnGetChildren(TSharedPtr<FAblAbilityTaskTreeNode> Entry, TArray< TSharedPtr<FAblAbilityTaskTreeNode> >& Children)
{
	if (Entry.IsValid())
	{
		Children.Append(Entry->Children);
	}
}

void SAblAbilityTaskPicker::OnSelectionChanged(TSharedPtr<FAblAbilityTaskTreeNode> Entry, ESelectInfo::Type SelectInfo)
{
	if (Entry.IsValid() && Entry->IsLeaf())
	{
		m_TaskClass = Entry->Task;
	}
}

void SAblAbilityTaskPicker::OnItemDoubleClicked(TSharedPtr<FAblAbilityTaskTreeNode> Entry)
{
	if (Entry.IsValid() && Entry->IsLeaf())
	{
		m_TaskClass = Entry->Task;
		m_bOkClicked = true;
		CloseDialog();
	}
}

void SAblAbilityTaskPicker::SetItemExpansion(TSharedPtr<FAblAbilityTaskTreeNode> Entry, bool bExpanded)
{
	if (Entry.IsValid() && !Entry->IsLeaf())
	{
		m_TaskTree->SetItemExpansion(Entry, bExpanded);

		for (TSharedPtr<FAblAbilityTaskTreeNode>& Child : Entry->Children)
		{
			if (!Child->IsLeaf())
			{
				m_TaskTree->SetItemExpansion(Child, bExpanded);
			}
		}
	}

}

FText SAblAbilityTaskPicker::GetTaskDescription() const
{
	if (const UAblAbilityTask* SelectedTask = m_TaskClass.GetDefaultObject())
	{
		return SelectedTask->GetTaskDescription();
	}

	return FText::GetEmpty();
}

void SAblAbilityTaskPicker::SortTree()
{
	for (TSharedPtr<FAblAbilityTaskTreeNode>& Node : m_TreeNodes)
	{
		TreeBuilderHelper::RecursiveTreeSort(Node);
	}

	// Sort the top level nodes themselves.
	m_TreeNodes.Sort([](const TSharedPtr<FAblAbilityTaskTreeNode>& A, const TSharedPtr<FAblAbilityTaskTreeNode>& B) -> bool
	{
		return A->CategoryName.CompareTo(B->CategoryName) < 0;
	});
}

void SAblAbilityTaskPicker::BuildTreeData()
{
	// Some of these are going to get reset again shortly, but I prefer to initialize them to some value anyway.
	TSharedPtr<FAblAbilityTaskTreeNode> ParentNode = nullptr;
	TSharedPtr<FAblAbilityTaskTreeNode> FoundNode = nullptr;
	
	m_TreeNodes.Empty();
	
	TArray<TSharedPtr<FAblAbilityTaskTreeNode>>* CurrentBranchLevel = &m_TreeNodes;
	
	UAblAbilityTask* Task = nullptr;
	
	TArray<FString> TaskCategories;


	for (TObjectIterator<UClass> ClassIter; ClassIter; ++ClassIter)
	{
		if (!ClassIter->IsChildOf(UAblAbilityTask::StaticClass()) || ClassIter->HasAnyClassFlags(CLASS_Abstract))
		{
			continue;
		}

		Task = Cast<UAblAbilityTask>(ClassIter->GetDefaultObject());

		if (Task->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) && (Task->GetClass()->GetName().Contains(TEXT("SKEL_")) || Task->GetClass()->GetName().Contains(TEXT("REINST"))))
		{
			continue;
		}

		if (Task != nullptr && !Task->GetTaskName().IsEmptyOrWhitespace())
		{
			// Reset everything before we iterate.
			ParentNode = nullptr;
			FoundNode = nullptr;
			CurrentBranchLevel = &m_TreeNodes;

			// If we have a name filter and our task name doesn't contain it, skip this class.
			if (!m_NameFilter.IsEmpty())
			{
				if (!Task->GetTaskName().ToString().Contains(m_NameFilter))
				{
					continue;
				}
			}

			FText CategoryText = Task->GetTaskCategory();
			CategoryText.ToString().ParseIntoArray(TaskCategories, TEXT("|"), true); // We support SubCategories using the | character.

			for (const FString& InCategory : TaskCategories)
			{
				FoundNode = TreeBuilderHelper::FindNodeInArray(*CurrentBranchLevel, InCategory);
				if (!FoundNode.IsValid())
				{
					// We didn't find the category. Add it to our current branch.
					FoundNode = MakeShareable(new FAblAbilityTaskTreeNode(FText::FromString(InCategory)));
					FoundNode->ParentNode = ParentNode;
					CurrentBranchLevel->Add(FoundNode);
				}
				else
				{
					ParentNode = FoundNode;
				}

				CurrentBranchLevel = &FoundNode->Children;
			}

			if (FoundNode.IsValid()) // We found our appropriate branch, make a leaf node
			{
				TSharedPtr<FAblAbilityTaskTreeNode> LeafNode = MakeShareable(new FAblAbilityTaskTreeNode(Task->GetClass()));
				LeafNode->ParentNode = FoundNode;
				FoundNode->Children.Add(LeafNode);
			}
		}
	}

	SortTree();
}

void SAblAbilityTaskPicker::MakeTaskClassPicker()
{
	m_TaskClassContainer->ClearChildren();

	m_TaskClassContainer->AddSlot()
		.AutoHeight()
		.VAlign(VAlign_Fill)
		.Padding(2.0f, 2.0f)
		[
			SAssignNew(m_TaskTree, STreeView<TSharedPtr<FAblAbilityTaskTreeNode>>)
			.TreeItemsSource(&m_TreeNodes)
			.OnSetExpansionRecursive(this, &SAblAbilityTaskPicker::SetItemExpansion)
			.OnGetChildren(this, &SAblAbilityTaskPicker::OnGetChildren)
			.OnGenerateRow(this, &SAblAbilityTaskPicker::OnGenerateRow)
			.OnMouseButtonDoubleClick(this, &SAblAbilityTaskPicker::OnItemDoubleClicked)
			.OnSelectionChanged(this, &SAblAbilityTaskPicker::OnSelectionChanged)
			.SelectionMode(ESelectionMode::Single)
			.HeaderRow
			(
				SNew(SHeaderRow)
				+ SHeaderRow::Column(Column_TaskName)
				.DefaultLabel(LOCTEXT("ColumnTaskName", "Name"))
				.FillWidth(0.5f)
			)
		];
}

void SAblAbilityTaskPicker::OnClassPicked(UClass* ChosenClass)
{
	m_TaskClass = ChosenClass;
}

FReply SAblAbilityTaskPicker::OkClicked()
{
	m_bOkClicked = true;

	CloseDialog();

	return FReply::Handled();
}

void SAblAbilityTaskPicker::CloseDialog()
{
	if (m_PickerWindow.IsValid())
	{
		m_PickerWindow.Pin()->RequestDestroyWindow();
	}
}

FReply SAblAbilityTaskPicker::CancelClicked()
{
	m_bOkClicked = false;
	
	CloseDialog();
	
	return FReply::Handled();
}

FReply SAblAbilityTaskPicker::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		m_bOkClicked = false;
		
		CloseDialog();
		
		return FReply::Handled();
	}

	return SWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

void SAblAbilityTaskTreeNodeWidget::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	m_Picker = InArgs._TreeWidget;
	m_TreeNode = InArgs._TreeNode;
	
	check(m_TreeNode.IsValid() && m_Picker.IsValid());

	SMultiColumnTableRow<TSharedPtr<FAblAbilityTaskTreeNode> >::Construct(SMultiColumnTableRow<TSharedPtr<FAblAbilityTaskTreeNode> >::FArguments(), InOwnerTableView);
}


TSharedRef<SWidget> SAblAbilityTaskTreeNodeWidget::GenerateWidgetForColumn(const FName& ColumnName)
{
	FSlateFontInfo FontStyle = m_TreeNode.Pin()->IsLeaf() ? FAppStyle::GetFontStyle("SmallText") : FAppStyle::GetFontStyle("NormalText");
	
	TSharedPtr<SHorizontalBox> Widget = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SExpanderArrow, SharedThis(this))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(STextBlock)
				.Font(FontStyle)
				.Text(this, &SAblAbilityTaskTreeNodeWidget::GetText)
			];

	return Widget.ToSharedRef();
}

FText SAblAbilityTaskTreeNodeWidget::GetText() const
{
	if (m_TreeNode.IsValid())
	{
		return m_TreeNode.Pin()->GetText();
	}

	return FText::FromString(TEXT("Unknown"));
}

#undef LOCTEXT_NAMESPACE