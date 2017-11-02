#include "ParticleEditorWidget.h"
#include "EmitterLayerWidget.h"
#include "LayerForceWidget.h"
#include "ParticleEmitterPropertiesWidget.h"
#include "Classes/Qt/Scene/SceneSignals.h"

#include <REPlatform/DataNodes/SelectionData.h>
#include <REPlatform/DataNodes/SceneData.h>
#include <REPlatform/Scene/Systems/EditorParticlesSystem.h>

#include <TArc/Core/Deprecated.h>
#include <TArc/Core/FieldBinder.h>

#include <Particles/ParticleEmitter.h>

#include <QScrollBar>

ParticleEditorWidget::ParticleEditorWidget(QWidget* parent /* = 0*/)
    : QScrollArea(parent)
{
    setWidgetResizable(true);

    emitterLayerWidget = NULL;
    layerForceWidget = NULL;
    emitterPropertiesWidget = NULL;
    effectPropertiesWidget = NULL;

    CreateInnerWidgets();

    auto dispatcher = SceneSignals::Instance();
    connect(dispatcher, &SceneSignals::ParticleLayerValueChanged, this, &ParticleEditorWidget::OnParticleLayerValueChanged);
    connect(dispatcher, &SceneSignals::ParticleEmitterLoaded, this, &ParticleEditorWidget::OnParticleEmitterLoaded);
    connect(dispatcher, &SceneSignals::ParticleEmitterSaved, this, &ParticleEditorWidget::OnParticleEmitterSaved);

    selectionFieldBinder.reset(new DAVA::FieldBinder(DAVA::Deprecated::GetAccessor()));
    {
        DAVA::FieldDescriptor fieldDescr;
        fieldDescr.type = DAVA::ReflectedTypeDB::Get<DAVA::SelectionData>();
        fieldDescr.fieldName = DAVA::FastName(DAVA::SelectionData::selectionPropertyName);
        selectionFieldBinder->BindField(fieldDescr, DAVA::MakeFunction(this, &ParticleEditorWidget::OnSelectionChanged));
    }
}

ParticleEditorWidget::~ParticleEditorWidget()
{
    DeleteInnerWidgets();
}

void ParticleEditorWidget::CreateInnerWidgets()
{
    effectPropertiesWidget = new ParticleEffectPropertiesWidget(this);
    effectPropertiesWidget->hide();

    emitterPropertiesWidget = new ParticleEmitterPropertiesWidget(this);
    emitterPropertiesWidget->hide();

    emitterLayerWidget = new EmitterLayerWidget(this);
    emitterLayerWidget->hide();

    layerForceWidget = new LayerForceWidget(this);
    layerForceWidget->hide();

    widgetMode = MODE_NONE;
}

void ParticleEditorWidget::DeleteInnerWidgets()
{
    SAFE_DELETE(effectPropertiesWidget);
    SAFE_DELETE(emitterPropertiesWidget);
    SAFE_DELETE(emitterLayerWidget);
    SAFE_DELETE(layerForceWidget);
}

void ParticleEditorWidget::OnUpdate()
{
    switch (widgetMode)
    {
    case MODE_EMITTER:
    {
        emitterPropertiesWidget->Update();
        break;
    }

    case MODE_LAYER:
    {
        emitterLayerWidget->Update(false);
        break;
    }

    case MODE_FORCE:
    {
        layerForceWidget->Update();
        break;
    }

    default:
    {
        break;
    }
    }
}

void ParticleEditorWidget::OnValueChanged()
{
    // Update the particle editor widgets when the value on the emitter layer is changed.
    UpdateParticleEditorWidgets();
}

void ParticleEditorWidget::UpdateParticleEditorWidgets()
{
    if (MODE_EMITTER == widgetMode && emitterPropertiesWidget->GetEmitterInstance(emitterPropertiesWidget->GetActiveScene()))
    {
        UpdateVisibleTimelinesForParticleEmitter();
        return;
    }

    if (MODE_LAYER == widgetMode && emitterLayerWidget->GetLayer())
    {
        UpdateWidgetsForLayer();
        return;
    }
}

void ParticleEditorWidget::UpdateVisibleTimelinesForParticleEmitter()
{
    // Safety check.
    if (MODE_EMITTER != widgetMode || !emitterPropertiesWidget->GetEmitterInstance(emitterPropertiesWidget->GetActiveScene()))
    {
        return;
    }

    // Update the visibility of particular timelines based on the emitter type.
    bool radiusTimeLineVisible = false;
    bool angleTimeLineVisible = false;
    bool sizeTimeLineVisible = false;

    auto emitterInstance = emitterPropertiesWidget->GetEmitterInstance(emitterPropertiesWidget->GetActiveScene());
    switch (emitterInstance->GetEmitter()->emitterType)
    {
    case DAVA::ParticleEmitter::EMITTER_ONCIRCLE_VOLUME:
    case DAVA::ParticleEmitter::EMITTER_ONCIRCLE_EDGES:
    case DAVA::ParticleEmitter::EMITTER_SHOCKWAVE:
    {
        radiusTimeLineVisible = true;
        angleTimeLineVisible = true;
        break;
    }

    case DAVA::ParticleEmitter::EMITTER_RECT:
    {
        sizeTimeLineVisible = true;
    }

    default:
    {
        break;
    }
    }

    emitterPropertiesWidget->GetEmitterRadiusTimeline()->setVisible(radiusTimeLineVisible);
    emitterPropertiesWidget->GetEmitterAngleTimeline()->setVisible(angleTimeLineVisible);
    emitterPropertiesWidget->GetEmitterSizeTimeline()->setVisible(sizeTimeLineVisible);
}

void ParticleEditorWidget::UpdateWidgetsForLayer()
{
    if (MODE_LAYER != widgetMode || !emitterLayerWidget->GetLayer())
    {
        return;
    }
    EmitterLayerWidget::eLayerMode mode = EmitterLayerWidget::eLayerMode::REGULAR;
    if (emitterLayerWidget->GetLayer()->type == DAVA::ParticleLayer::TYPE_SUPEREMITTER_PARTICLES)
        mode = EmitterLayerWidget::eLayerMode::SUPEREMITTER;
    else if (emitterLayerWidget->GetLayer()->type == DAVA::ParticleLayer::TYPE_PARTICLE_STRIPE)
        mode = EmitterLayerWidget::eLayerMode::STRIPE;

    emitterLayerWidget->SetLayerMode(mode);
}

void ParticleEditorWidget::HandleEmitterSelected(DAVA::SceneEditor2* scene, DAVA::ParticleEffectComponent* effect, DAVA::ParticleEmitterInstance* emitter, bool forceUpdate)
{
    auto widgetInstance = emitterPropertiesWidget->GetEmitterInstance(scene);
    auto sameEmitter = (widgetInstance != nullptr) && (widgetInstance->GetEmitter() == emitter->GetEmitter());
    if ((emitter != nullptr) && (MODE_EMITTER == widgetMode) && (!forceUpdate && sameEmitter))
    {
        return;
    }

    SwitchEditorToEmitterMode(scene, effect, emitter);
}

void ParticleEditorWidget::OnSelectionChanged(const DAVA::Any& selectionAny)
{
    if (selectionAny.CanGet<DAVA::SelectableGroup>())
    {
        const DAVA::SelectableGroup& selection = selectionAny.Get<DAVA::SelectableGroup>();

        DAVA::SceneData* sceneData = DAVA::Deprecated::GetActiveDataNode<DAVA::SceneData>();
        DAVA::SceneEditor2* scene = sceneData->GetScene().Get();

        ProcessSelection(scene, selection);
    }
    else
    {
        ResetEditorMode();
    }
}

void ParticleEditorWidget::ProcessSelection(DAVA::SceneEditor2* scene, const DAVA::SelectableGroup& selection)
{
    bool shouldReset = true;
    SCOPE_EXIT
    {
        if (shouldReset)
        {
            ResetEditorMode();
        }
    };

    if (selection.GetSize() != 1)
        return;

    const auto& obj = selection.GetFirst();
    if (obj.CanBeCastedTo<DAVA::Entity>())
    {
        DAVA::Entity* entity = obj.AsEntity();
        DAVA::ParticleEffectComponent* effect = static_cast<DAVA::ParticleEffectComponent*>(entity->GetComponent(DAVA::Component::PARTICLE_EFFECT_COMPONENT));
        if (effect != nullptr)
        {
            shouldReset = false;
            SwitchEditorToEffectMode(scene, effect);
        }
    }
    else if (obj.CanBeCastedTo<DAVA::ParticleEmitterInstance>())
    {
        shouldReset = false;
        DAVA::ParticleEmitterInstance* instance = obj.Cast<DAVA::ParticleEmitterInstance>();
        SwitchEditorToEmitterMode(scene, instance->GetOwner(), instance);
    }
    else if (obj.CanBeCastedTo<DAVA::ParticleLayer>())
    {
        DAVA::ParticleLayer* layer = obj.Cast<DAVA::ParticleLayer>();
        DAVA::ParticleEmitterInstance* instance = scene->GetSystem<DAVA::EditorParticlesSystem>()->GetLayerOwner(layer);
        if (instance != nullptr)
        {
            shouldReset = false;
            SwitchEditorToLayerMode(scene, instance->GetOwner(), instance, layer);
        }
    }
    else if (obj.CanBeCastedTo<DAVA::ParticleForce>())
    {
        DAVA::ParticleForce* force = obj.Cast<DAVA::ParticleForce>();
        DAVA::ParticleLayer* layer = scene->GetSystem<DAVA::EditorParticlesSystem>()->GetForceOwner(force);
        if (layer != nullptr)
        {
            auto i = std::find(layer->forces.begin(), layer->forces.end(), force);
            if (i != layer->forces.end())
            {
                shouldReset = false;
                SwitchEditorToForceMode(scene, layer, std::distance(layer->forces.begin(), i));
            }
        }
    }
}

void ParticleEditorWidget::OnParticleLayerValueChanged(DAVA::SceneEditor2* /*scene*/, DAVA::ParticleLayer* layer)
{
    if (MODE_LAYER != widgetMode || emitterLayerWidget->GetLayer() != layer)
    {
        return;
    }

    // Notify the Emitter Layer widget about its inner layer value is changed and
    // the widget needs to be resynchronized with its values.
    emitterLayerWidget->OnLayerValueChanged();
    emitterLayerWidget->Update(false);
}

void ParticleEditorWidget::OnParticleEmitterLoaded(DAVA::SceneEditor2* scene, DAVA::ParticleEmitterInstance* emitter)
{
    // Handle in the same way emitter is selected to update the values. However
    // cause widget to be force updated.
    HandleEmitterSelected(scene, emitterPropertiesWidget->GetEffect(scene), emitter, true);
}

void ParticleEditorWidget::OnParticleEmitterSaved(DAVA::SceneEditor2* scene, DAVA::ParticleEmitterInstance* emitter)
{
    // Handle in the same way emitter is selected to update the values. However
    // cause widget to be force updated.
    DAVA::ParticleEffectComponent* currEffect = emitterPropertiesWidget->GetEffect(scene);
    DAVA::ParticleEmitterInstance* currEmitter = emitterPropertiesWidget->GetEmitterInstance(scene);
    if (currEffect && (currEmitter->GetEmitter() == emitter->GetEmitter()))
    {
        HandleEmitterSelected(scene, currEffect, emitter, true);
    }
}

void ParticleEditorWidget::SwitchEditorToEffectMode(DAVA::SceneEditor2* scene, DAVA::ParticleEffectComponent* effect)
{
    ResetEditorMode();

    if (!effect)
    {
        emit ChangeVisible(false);
        return;
    }

    emit ChangeVisible(true);

    effectPropertiesWidget->Init(scene, effect);
    setWidget(effectPropertiesWidget);
    effectPropertiesWidget->show();

    this->widgetMode = MODE_EFFECT;
}

void ParticleEditorWidget::SwitchEditorToEmitterMode(DAVA::SceneEditor2* scene, DAVA::ParticleEffectComponent* effect, DAVA::ParticleEmitterInstance* emitter)
{
    ResetEditorMode();

    if (!emitter)
    {
        emit ChangeVisible(false);
        return;
    }

    emit ChangeVisible(true);
    this->widgetMode = MODE_EMITTER;

    emitterPropertiesWidget->Init(scene, effect, emitter, true);
    setWidget(emitterPropertiesWidget);
    emitterPropertiesWidget->show();

    connect(emitterPropertiesWidget,
            SIGNAL(ValueChanged()),
            this,
            SLOT(OnValueChanged()));

    UpdateParticleEditorWidgets();
}

void ParticleEditorWidget::SwitchEditorToLayerMode(DAVA::SceneEditor2* scene, DAVA::ParticleEffectComponent* effect,
                                                   DAVA::ParticleEmitterInstance* emitter, DAVA::ParticleLayer* layer)
{
    ResetEditorMode();

    if (!emitter || !layer)
    {
        emit ChangeVisible(false);
        return;
    }

    emit ChangeVisible(true);
    this->widgetMode = MODE_LAYER;

    emitterLayerWidget->Init(scene, effect, emitter, layer, true);
    setWidget(emitterLayerWidget);
    emitterLayerWidget->show();

    connect(emitterLayerWidget,
            SIGNAL(ValueChanged()),
            this,
            SLOT(OnValueChanged()));

    UpdateParticleEditorWidgets();
}

void ParticleEditorWidget::SwitchEditorToForceMode(DAVA::SceneEditor2* scene, DAVA::ParticleLayer* layer, DAVA::int32 forceIndex)
{
    ResetEditorMode();

    if (!layer)
    {
        emit ChangeVisible(false);
        return;
    }

    emit ChangeVisible(true);
    widgetMode = MODE_FORCE;

    layerForceWidget->Init(scene, layer, forceIndex, true);
    setWidget(layerForceWidget);
    layerForceWidget->show();
    connect(layerForceWidget, SIGNAL(ValueChanged()), this, SLOT(OnValueChanged()));

    UpdateParticleEditorWidgets();
}

void ParticleEditorWidget::ResetEditorMode()
{
    QWidget* activeEditorWidget = takeWidget();
    if (!activeEditorWidget)
    {
        DVASSERT(widgetMode == MODE_NONE);
        return;
    }

    switch (widgetMode)
    {
    case MODE_EFFECT:
    {
        effectPropertiesWidget = static_cast<ParticleEffectPropertiesWidget*>(activeEditorWidget);
        effectPropertiesWidget->hide();

        break;
    }

    case MODE_EMITTER:
    {
        emitterPropertiesWidget = static_cast<ParticleEmitterPropertiesWidget*>(activeEditorWidget);
        disconnect(emitterPropertiesWidget, SIGNAL(ValueChanged()), this, SLOT(OnValueChanged()));
        emitterPropertiesWidget->hide();

        break;
    }

    case MODE_LAYER:
    {
        emitterLayerWidget = static_cast<EmitterLayerWidget*>(activeEditorWidget);
        disconnect(emitterLayerWidget, SIGNAL(ValueChanged()), this, SLOT(OnValueChanged()));
        emitterLayerWidget->hide();

        break;
    }

    case MODE_FORCE:
    {
        layerForceWidget = static_cast<LayerForceWidget*>(activeEditorWidget);
        disconnect(layerForceWidget, SIGNAL(ValueChanged()), this, SLOT(OnValueChanged()));
        layerForceWidget->hide();

        break;
    }

    default:
    {
        break;
    }
    }

    widgetMode = MODE_NONE;
}
