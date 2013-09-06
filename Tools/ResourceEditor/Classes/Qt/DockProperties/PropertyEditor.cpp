/*==================================================================================
    Copyright (c) 2008, binaryzebra
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the binaryzebra nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE binaryzebra AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL binaryzebra BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=====================================================================================*/



#include "DAVAEngine.h"
#include "Scene/SceneDataManager.h"
#include "Entity/Component.h"
#include "Main/mainwindow.h"

#include <QPushButton>

#include "DockProperties/PropertyEditor.h"
#include "Tools/QtPropertyEditor/QtPropertyItem.h"
#include "Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataIntrospection.h"
#include "Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataDavaVariant.h"
#include "Commands2/MetaObjModifyCommand.h"
#include "Commands2/InspMemberModifyCommand.h"

#include "PropertyEditorStateHelper.h"

#include "ActionComponentEditor.h"
PropertyEditor::PropertyEditor(QWidget *parent /* = 0 */, bool connectToSceneSignals /*= true*/)
	: QtPropertyEditor(parent)
	, advancedMode(false)
	, curNode(NULL)
	, treeStateHelper(this, this->curModel)
{
	if(connectToSceneSignals)
	{
		QObject::connect(SceneSignals::Instance(), SIGNAL(Selected(SceneEditor2 *, DAVA::Entity *)), this, SLOT(EntitySelected(SceneEditor2 *, DAVA::Entity *)));
		QObject::connect(SceneSignals::Instance(), SIGNAL(Deselected(SceneEditor2 *, DAVA::Entity *)), this, SLOT(EntityDeselected(SceneEditor2 *, DAVA::Entity *)));
		QObject::connect(SceneSignals::Instance(), SIGNAL(Activated(SceneEditor2 *)), this, SLOT(sceneActivated(SceneEditor2 *)));
		QObject::connect(SceneSignals::Instance(), SIGNAL(Deactivated(SceneEditor2 *)), this, SLOT(sceneDeactivated(SceneEditor2 *)));

		// MainWindow actions
		QObject::connect(QtMainWindow::Instance()->GetUI()->actionShowAdvancedProp, SIGNAL(triggered()), this, SLOT(actionShowAdvanced()));
		advancedMode = QtMainWindow::Instance()->GetUI()->actionShowAdvancedProp->isChecked();
	}
	posSaver.Attach(this, "DocPropetyEditor");

	DAVA::VariantType v = posSaver.LoadValue("splitPos");
	if(v.GetType() == DAVA::VariantType::TYPE_INT32) header()->resizeSection(0, v.AsInt32());

	SetRefreshTimeout(5000);
	SetEditTracking(true);
}

PropertyEditor::~PropertyEditor()
{
	DAVA::VariantType v(header()->sectionSize(0));
	posSaver.SaveValue("splitPos", v);

	SafeRelease(curNode);
}

void PropertyEditor::SetNode(DAVA::Entity *node)
{
	// Store the current Property Editor Tree state before switching to the new node.
	// Do not clear the current states map - we are using one storage to share opened
	// Property Editor nodes between the different Scene Nodes.
	treeStateHelper.SaveTreeViewState(false);
	
	SafeRelease(curNode);
	curNode = SafeRetain(node);

	RemovePropertyAll();
	if(NULL != curNode)
	{
        AppendIntrospectionInfo(curNode, curNode->GetTypeInfo());

		for(int32 i = 0; i < Component::COMPONENT_COUNT; ++i)
        {
            Component *component = curNode->GetComponent(i);
            if(component)
            {
                QtPropertyData *componentData = AppendIntrospectionInfo(component, component->GetTypeInfo());

				if(NULL != componentData)
				{
					// Add optional button to track "remove this component" command
					QPushButton *removeButton = new QPushButton(QIcon(":/QtIcons/removecomponent.png"), "");
					removeButton->setFlat(true);

					componentData->AddOW(QtPropertyOW(removeButton, true));
				}
            }
        }
	}

	// Restore back the tree view state from the shared storage.
	if (!treeStateHelper.IsTreeStateStorageEmpty())
	{
		treeStateHelper.RestoreTreeViewState();
	}
	else
	{
		// Expand the root elements as default value.
		expandToDepth(0);
	}
}

void PropertyEditor::SetAdvancedMode(bool set)
{
	if(advancedMode != set)
	{
		advancedMode = set;
		SetNode(curNode);
	}
}

QtPropertyData* PropertyEditor::AppendIntrospectionInfo(void *object, const DAVA::InspInfo *info)
{
	QtPropertyData* propData = NULL;

	if(NULL != info)
	{
		bool hasMembers = false;
		const InspInfo *currentInfo = info;

		// check if there are any memebers
		while (NULL != currentInfo)
		{
			if(currentInfo->MembersCount() > 0)
			{
				hasMembers = true;
				break;
			}
			currentInfo = currentInfo->BaseInfo();
		}

        if(hasMembers)
        {
			int flags = DAVA::I_VIEW;

			// in basic mode show only field that can be viewed and edited
			if(!advancedMode)
			{
				flags |= DAVA::I_EDIT;
			}

			propData = new QtPropertyDataIntrospection(object, currentInfo, flags);
			if(propData->ChildCount() > 0)
			{
				QPair<QtPropertyItem*, QtPropertyItem*> prop = AppendProperty(currentInfo->Name(), propData);
            
	            prop.first->setBackground(QBrush(QColor(Qt::lightGray)));
		        prop.second->setBackground(QBrush(QColor(Qt::lightGray)));
			}
			else
			{
				delete propData;
				propData = NULL;
			}
        }
    }

	return propData;
}

void PropertyEditor::sceneActivated(SceneEditor2 *scene)
{
	if(NULL != scene)
	{
		SetNode(scene->selectionSystem->GetSelection()->GetEntity(0));
	}
}

void PropertyEditor::sceneDeactivated(SceneEditor2 *scene)
{
	SetNode(NULL);
}

void PropertyEditor::actionShowAdvanced()
{
	QAction *showAdvancedAction = dynamic_cast<QAction *>(QObject::sender());
	if(NULL != showAdvancedAction)
	{
		SetAdvancedMode(showAdvancedAction->isChecked());
	}
}


void PropertyEditor::EntitySelected(SceneEditor2 *scene, DAVA::Entity *entity)
{
	SetNode(entity);
}

void PropertyEditor::EntityDeselected(SceneEditor2 *scene, DAVA::Entity *entity)
{

}

void PropertyEditor::OnItemEdited(const QString &name, QtPropertyData *data)
{
	QtPropertyEditor::OnItemEdited(name, data);

	Command2 *command = (Command2 *) data->CreateLastCommand();
	if(NULL != command)
	{
		QtMainWindow::Instance()->GetCurrentScene()->Exec(command);
	}
}
