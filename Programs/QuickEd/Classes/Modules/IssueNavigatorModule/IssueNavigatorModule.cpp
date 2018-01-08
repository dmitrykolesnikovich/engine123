#include "Classes/Modules/IssueNavigatorModule/IssueNavigatorModule.h"

#include "Modules/IssueNavigatorModule/IssueData.h"
#include "Modules/IssueNavigatorModule/LayoutIssueHandler.h"
#include "Modules/IssueNavigatorModule/DataBindingIssueHandler.h"
#include "Modules/IssueNavigatorModule/NamingIssuesHandler.h"
#include "Modules/IssueNavigatorModule/EventsIssuesHandler.h"

#include "Application/QEGlobal.h"

#include <TArc/WindowSubSystem/UI.h>
#include <TArc/Utils/ModuleCollection.h>
#include <TArc/Controls/TableView.h>

DAVA_VIRTUAL_REFLECTION_IMPL(IssueNavigatorModule)
{
    DAVA::ReflectionRegistrator<IssueNavigatorModule>::Begin()
    .ConstructorByPointer()
    .Field("header", &IssueNavigatorModule::headerDescription)
    .Field("issues", &IssueNavigatorModule::GetValues, nullptr)
    .Field("current", &IssueNavigatorModule::GetCurrentValue, &IssueNavigatorModule::SetCurrentValue)
    .Method("OnIssueActivated", &IssueNavigatorModule::OnIssueAvitvated)
    .End();
}

DAVA_REFLECTION_IMPL(IssueNavigatorModule::HeaderDescription)
{
    DAVA::ReflectionRegistrator<IssueNavigatorModule::HeaderDescription>::Begin()
    .Field("message", &IssueNavigatorModule::HeaderDescription::message)[DAVA::M::DisplayName("Message")]
    .Field("pathToControl", &IssueNavigatorModule::HeaderDescription::pathToControl)[DAVA::M::DisplayName("Path To Control")]
    .Field("packagePath", &IssueNavigatorModule::HeaderDescription::packagePath)[DAVA::M::DisplayName("Package Path")]
    .Field("propertyName", &IssueNavigatorModule::HeaderDescription::propertyName)[DAVA::M::DisplayName("Property Name")]
    .End();
}

void IssueNavigatorModule::PostInit()
{
    using namespace DAVA::TArc;
    const char* title = "Issue Navigator";
    DockPanelInfo panelInfo;
    panelInfo.title = title;
    panelInfo.area = Qt::BottomDockWidgetArea;
    PanelKey key(title, panelInfo);

    TableView::Params params(GetAccessor(), GetUI(), DAVA::TArc::mainWindowKey);
    params.fields[TableView::Fields::Header] = "header";
    params.fields[TableView::Fields::Values] = "issues";
    params.fields[TableView::Fields::CurrentValue] = "current";
    params.fields[TableView::Fields::ItemActivated] = "OnIssueActivated";

    TableView* tableView = new TableView(params, GetAccessor(), DAVA::Reflection::Create(DAVA::ReflectedObject(this)));
    GetUI()->AddView(DAVA::TArc::mainWindowKey, key, tableView->ToWidgetCast());

    DAVA::int32 sectionId = 0;
    handlers.push_back(std::make_unique<LayoutIssueHandler>(GetAccessor(), GetUI(), sectionId++, indexGenerator));
    handlers.push_back(std::make_unique<DataBindingIssueHandler>(GetAccessor(), sectionId++, indexGenerator));
    handlers.push_back(std::make_unique<NamingIssuesHandler>(GetAccessor(), GetUI(), sectionId++, indexGenerator));
    handlers.push_back(std::make_unique<EventsIssuesHandler>(GetAccessor(), GetUI(), sectionId++, indexGenerator));
}

void IssueNavigatorModule::OnWindowClosed(const DAVA::TArc::WindowKey& key)
{
    if (key == DAVA::TArc::mainWindowKey)
    {
        handlers.clear();
    }
}

void IssueNavigatorModule::OnContextCreated(DAVA::TArc::DataContext* context)
{
    context->CreateData(std::make_unique<IssueData>());
}

void IssueNavigatorModule::OnContextWillBeChanged(DAVA::TArc::DataContext* current, DAVA::TArc::DataContext* newOne)
{
    if (current != nullptr)
    {
        IssueData* issueData = current->GetData<IssueData>();
        issueData->RemoveAllIssues();
    }
}

void IssueNavigatorModule::OnIssueAvitvated(size_t index)
{
    const DAVA::TArc::DataContext* context = GetAccessor()->GetActiveContext();
    if (context)
    {
        DVASSERT(index != static_cast<size_t>(-1));

        IssueData* issueData = context->GetData<IssueData>();
        if (issueData)
        {
            const IssueData::Issue& issue = issueData->GetIssues()[index];
            const QString& path = QString::fromStdString(DAVA::FilePath(issue.packagePath).GetAbsolutePathname());
            const QString& name = QString::fromStdString(issue.pathToControl);
            InvokeOperation(QEGlobal::SelectControl.ID, path, name);
        }
    }
}

const DAVA::Vector<IssueData::Issue>& IssueNavigatorModule::GetValues() const
{
    const DAVA::TArc::DataContext* context = GetAccessor()->GetActiveContext();
    if (context)
    {
        IssueData* issueData = context->GetData<IssueData>();
        DVASSERT(issueData);
        return issueData->GetIssues();
    }

    static DAVA::Vector<IssueData::Issue> empty;
    return empty;
}

void IssueNavigatorModule::OnContextWasChanged(DAVA::TArc::DataContext* current, DAVA::TArc::DataContext* oldOne)
{
    for (const std::unique_ptr<IssueHandler>& handler : handlers)
    {
        handler->OnContextActivated(current);
    }
}

void IssueNavigatorModule::OnContextDeleted(DAVA::TArc::DataContext* context)
{
    for (const std::unique_ptr<IssueHandler>& handler : handlers)
    {
        handler->OnContextDeleted(context);
    }
}

DAVA::Any IssueNavigatorModule::GetCurrentValue() const
{
    return current;
}

void IssueNavigatorModule::SetCurrentValue(const DAVA::Any& currentValue)
{
    if (currentValue.IsEmpty())
    {
        current = static_cast<size_t>(0);
    }
    else
    {
        current = currentValue.Cast<size_t>();
    }
}

DECL_GUI_MODULE(IssueNavigatorModule);
