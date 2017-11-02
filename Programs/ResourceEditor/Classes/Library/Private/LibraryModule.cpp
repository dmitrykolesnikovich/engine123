#include "Classes/Library/LibraryModule.h"
#include "Classes/Library/Private/ControlsFactory.h"
#include "Classes/Library/Private/LibraryData.h"
#include "Classes/Library/Private/LibraryWidget.h"
#include "Classes/Library/Private/DAEConverter.h"

#include <REPlatform/DataNodes/Settings/RESettings.h>

#include <TArc/Utils/ModuleCollection.h>
#include <TArc/Core/FieldBinder.h>
#include <TArc/WindowSubSystem/ActionUtils.h>
#include <TArc/WindowSubSystem/UI.h>
#include <TArc/WindowSubSystem/QtAction.h>

#include <FileSystem/FilePath.h>
#include <Functional/Function.h>
#include <Reflection/ReflectionRegistrator.h>
#include "REPlatform/Global/GlobalOperations.h"

LibraryModule::~LibraryModule()
{
    ControlsFactory::ReleaseFonts();
}

void LibraryModule::PostInit()
{
    std::unique_ptr<LibraryData> libraryData = std::make_unique<LibraryData>();
    GetAccessor()->GetGlobalContext()->CreateData(std::move(libraryData));

    LibraryWidget* libraryWidget = new LibraryWidget(GetAccessor(), nullptr);
    connections.AddConnection(libraryWidget, &LibraryWidget::AddSceneRequested, DAVA::MakeFunction(this, &LibraryModule::OnAddSceneRequested));
    connections.AddConnection(libraryWidget, &LibraryWidget::EditSceneRequested, DAVA::MakeFunction(this, &LibraryModule::OnEditSceneRequested));
    connections.AddConnection(libraryWidget, &LibraryWidget::DAEConvertionRequested, DAVA::MakeFunction(this, &LibraryModule::OnDAEConvertionRequested));
    connections.AddConnection(libraryWidget, &LibraryWidget::DAEAnimationConvertionRequested, DAVA::MakeFunction(this, &LibraryModule::OnDAEAnimationConvertionRequested));
    connections.AddConnection(libraryWidget, &LibraryWidget::DoubleClicked, DAVA::MakeFunction(this, &LibraryModule::OnDoubleClicked));
    connections.AddConnection(libraryWidget, &LibraryWidget::DragStarted, DAVA::MakeFunction(this, &LibraryModule::OnDragStarted));

    DAVA::DockPanelInfo dockInfo;
    dockInfo.title = "Library";
    DAVA::PanelKey panelKey(QStringLiteral("LibraryDock"), dockInfo);
    GetUI()->AddView(DAVA::mainWindowKey, panelKey, libraryWidget);

    fieldBinder.reset(new DAVA::FieldBinder(GetAccessor()));
    DAVA::FieldDescriptor libraryFieldDescriptor(DAVA::ReflectedTypeDB::Get<LibraryData>(), DAVA::FastName(LibraryData::selectedPathProperty));
    fieldBinder->BindField(libraryFieldDescriptor, DAVA::MakeFunction(this, &LibraryModule::OnSelectedPathChanged));
}

void LibraryModule::OnSelectedPathChanged(const DAVA::Any& selectedPathValue)
{
    DAVA::GeneralSettings* settings = GetAccessor()->GetGlobalContext()->GetData<DAVA::GeneralSettings>();
    if (settings->previewEnabled == true)
    {
        DAVA::FilePath selectedPath;
        if (selectedPathValue.CanGet<DAVA::FilePath>())
        {
            selectedPath = selectedPathValue.Get<DAVA::FilePath>();
        }

        if (selectedPath.IsEqualToExtension(".sc2"))
        {
            ShowPreview(selectedPath);
        }
        else
        {
            HidePreview();
        }
    }
}

void LibraryModule::ShowPreview(const DAVA::FilePath& path)
{
    if (previewDialog.Get() == nullptr)
    {
        previewDialog.Set(new ScenePreviewDialog());
    }

    previewDialog->Show(path);
}

void LibraryModule::HidePreview()
{
    if (previewDialog.Get() != nullptr && previewDialog->GetParent())
    {
        previewDialog->Close();
    }
}

void LibraryModule::OnAddSceneRequested(const DAVA::FilePath& scenePathname)
{
    HidePreview();

    InvokeOperation(DAVA::AddSceneOperation.ID, scenePathname);
}

void LibraryModule::OnEditSceneRequested(const DAVA::FilePath& scenePathname)
{
    HidePreview();

    InvokeOperation(DAVA::OpenSceneOperation.ID, scenePathname);
}

void LibraryModule::OnDAEConvertionRequested(const DAVA::FilePath& daePathname)
{
    HidePreview();

    executor.DelayedExecute([this, daePathname]() {
        DAVA::UI* ui = GetUI();
        DAVA::WaitDialogParams waitDlgParams;
        waitDlgParams.message = QString("DAE to SC2 conversion\n%1").arg(daePathname.GetAbsolutePathname().c_str());
        waitDlgParams.needProgressBar = false;
        std::unique_ptr<DAVA::WaitHandle> waitHandle = ui->ShowWaitDialog(DAVA::mainWindowKey, waitDlgParams);

        DAEConverter::Convert(daePathname);
    });
}

void LibraryModule::OnDAEAnimationConvertionRequested(const DAVA::FilePath& daePathname)
{
    HidePreview();

    executor.DelayedExecute([this, daePathname]() {
        DAVA::UI* ui = GetUI();
        DAVA::WaitDialogParams waitDlgParams;
        waitDlgParams.message = QString("DAE animations conversion\n%1").arg(daePathname.GetAbsolutePathname().c_str());
        waitDlgParams.needProgressBar = false;
        std::unique_ptr<DAVA::WaitHandle> waitHandle = ui->ShowWaitDialog(DAVA::mainWindowKey, waitDlgParams);

        DAEConverter::ConvertAnimations(daePathname);
    });
}

void LibraryModule::OnDoubleClicked(const DAVA::FilePath& scenePathname)
{
    HidePreview();

    DAVA::GeneralSettings* settings = GetAccessor()->GetGlobalContext()->GetData<DAVA::GeneralSettings>();
    if (scenePathname.IsEqualToExtension(".sc2"))
    {
        OnEditSceneRequested(scenePathname);
    }
}

void LibraryModule::OnDragStarted()
{
    HidePreview();
}

DAVA_VIRTUAL_REFLECTION_IMPL(LibraryModule)
{
    DAVA::ReflectionRegistrator<LibraryModule>::Begin()
    .ConstructorByPointer()
    .End();
}

DECL_GUI_MODULE(LibraryModule);
