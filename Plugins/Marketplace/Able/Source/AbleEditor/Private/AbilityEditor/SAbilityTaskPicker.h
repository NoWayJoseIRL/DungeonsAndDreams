// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Tasks/IAblAbilityTask.h"

#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STreeView.h"
#include "Widgets/SWindow.h"

class UAblAbility;

/* This struct represents a single node on the Task tree. */
struct FAblAbilityTaskTreeNode
{
public:
	/* Parent of this node, nullptr if it's a top level node. */
	TSharedPtr<FAblAbilityTaskTreeNode> ParentNode;

	/* Holds the child nodes which could simply be a category/sub-category, or a leaf. */
	TArray<TSharedPtr<FAblAbilityTaskTreeNode>> Children;

	/* Holds the display name text. */
	FText CategoryName;
	 
	/* If not nullptr, this is a leaf node. **/
	TSubclassOf<UAblAbilityTask> Task;

public:
	/* Returns whether or not this Node is a leaf node. */
	bool IsLeaf() const { return Task.GetDefaultObject() != nullptr; }

	/* Returns the text for this node. */
	const FText GetText() const;

	/**
	* Creates and initializes a new instance.
	*
	* @param InLabel The node's label.
	*/
	FAblAbilityTaskTreeNode(const FText& Category)
		: CategoryName(Category)
	{ }

	FAblAbilityTaskTreeNode(UClass* InClass)
		:Task(InClass)
	{ }
};

/* Ability Task Picker is a window that allows the user to select from all the classes that inherit from UAblAbilityTask. 
 * It supports things like Categories and Sub Categories, as well as searching by name. */
class SAblAbilityTaskPicker : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAblAbilityTaskPicker)
	{
	}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/* Construct and display the window. Returns true if a class was selected, or false if not. */
	bool DoModal();

	/* Returns the Task class selected. */
	const TSubclassOf<UAblAbilityTask>& GetTaskClass() const { return m_TaskClass; }

	/* Callback for when the user chooses a Task class. */
	void SetTaskClass(UClass* TaskClass) { m_TaskClass = TaskClass; }
private:
	/* Callback for when the Filter is modified. */
	void OnFilterStringChanged(const FText& InFilterString);

	/* Callback to generate a Row for the tree. */
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FAblAbilityTaskTreeNode> Entry, const TSharedRef<STableViewBase>& Table);

	/* Callback to Get the children for a specific node. */
	void OnGetChildren(TSharedPtr<FAblAbilityTaskTreeNode> Entry, TArray< TSharedPtr<FAblAbilityTaskTreeNode> >& Children);

	/* Callback when the user changes their tree selection in some way. */
	void OnSelectionChanged(TSharedPtr<FAblAbilityTaskTreeNode> Entry, ESelectInfo::Type SelectInfo);

	/* Callback when an item on the tree is double-clicked. */
	void OnItemDoubleClicked(TSharedPtr<FAblAbilityTaskTreeNode> Entry);

	/* Sets the expansion state of a tree item. */
	void SetItemExpansion(TSharedPtr<FAblAbilityTaskTreeNode> Entry, bool bExpanded);

	/* Returns the Task description for a Task. */
	FText GetTaskDescription() const;

	/* Sorts the tree alphabetically. */
	void SortTree();

	/* Builds the tree according to the filter (or lack there of). */
	void BuildTreeData();

	/* Creates the picker window. */
	void MakeTaskClassPicker();

	/* Callback when a class is selected. */
	void OnClassPicked(UClass* ChosenClass);

	/* Callback when the OK button is clicked. */
	FReply OkClicked();

	/* Closes the window. */
	void CloseDialog();

	/* Callback when the Cancel button is clicked. */
	FReply CancelClicked();

	// Input Override
	FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);

	/* A pointer to the window that is asking the user to select a parent class */
	TWeakPtr<SWindow> m_PickerWindow;

	/* Our Filter widget. */
	TSharedPtr<SSearchBox> m_SearchFilter;

	/* Flattened Array of all nodes in the tree. */
	TArray<TSharedPtr<FAblAbilityTaskTreeNode>> m_TreeNodes;

	/* Tree View Widget. */
	TSharedPtr<STreeView<TSharedPtr<FAblAbilityTaskTreeNode>>> m_TaskTree;

	/* The container for the Parent Class picker */
	TSharedPtr<SVerticalBox> m_TaskClassContainer;

	/* The selected class */
	TSubclassOf<UAblAbilityTask> m_TaskClass;

	/* Current Filter string. */
	FString m_NameFilter;

	/* True if Ok was clicked */
	bool m_bOkClicked;

	/* Name of our Column for the Tree view. */
	static const FName Column_TaskName;

};

/* Widget used for displaying a row in our Tree view. */
class SAblAbilityTaskTreeNodeWidget : public SMultiColumnTableRow< TSharedPtr<FAblAbilityTaskTreeNode> >
{
	SLATE_BEGIN_ARGS(SAblAbilityTaskTreeNodeWidget)
	{

	}
	SLATE_ARGUMENT(TSharedPtr<SAblAbilityTaskPicker>, TreeWidget)
	SLATE_ARGUMENT(TSharedPtr<FAblAbilityTaskTreeNode>, TreeNode)
	SLATE_END_ARGS()
public:

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);

	/* Returns the Widget for this Column.*/
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:
	/* Returns the Text for this column.*/
	FText GetText() const;

	/* Pointer back to our Picker. */
	TSharedPtr<SAblAbilityTaskPicker> m_Picker;

	/* The tree node this widget represents. */
	TWeakPtr<FAblAbilityTaskTreeNode> m_TreeNode;

	/* Container for our widget. */
	TSharedPtr<SVerticalBox> m_Widget;
};