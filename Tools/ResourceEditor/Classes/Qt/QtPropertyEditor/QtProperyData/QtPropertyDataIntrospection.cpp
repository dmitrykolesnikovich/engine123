#include "DAVAEngine.h"
#include "QtPropertyEditor/QtProperyData/QtPropertyDataIntrospection.h"
#include "QtPropertyEditor/QtProperyData/QtPropertyDataDavaVariant.h"

QtPropertyDataIntrospection::QtPropertyDataIntrospection(void *_object, const DAVA::IntrospectionInfo *_info)
	: object(_object)
	, info(_info)
{
	if(NULL != info)
	{
		for(int i = 0; i < info->MembersCount(); ++i)
		{
			const DAVA::IntrospectionMember *member = info->Member(i);
			if(NULL != member)
			{
				const DAVA::MetaInfo *memberMetaInfo = member->Type();

				// check if member has introspection
				if(NULL != memberMetaInfo->Introspection())
				{
					QtPropertyData *childData = new QtPropertyDataIntrospection(member->Pointer(object), memberMetaInfo->Introspection());
					ChildAdd(info->Member(i)->Name(), childData);
					childVariantIndexes.insert(NULL, i);
				}
				else
				{
					QtPropertyDataDavaVariant *childData = new QtPropertyDataDavaVariant(info->Member(i)->Value(object));
					ChildAdd(info->Member(i)->Name(), childData);
					childVariantIndexes.insert(childData, i);
				}
			}
		}
	}

	SetFlags(FLAG_IS_DISABLED);
}

QtPropertyDataIntrospection::~QtPropertyDataIntrospection()
{ }

QVariant QtPropertyDataIntrospection::GetValueInternal()
{
	ChildNeedUpdate();
	return QVariant();
}

void QtPropertyDataIntrospection::ChildChanged(const QString &key, QtPropertyData *data)
{
	QtPropertyDataDavaVariant *dataVariant = (QtPropertyDataDavaVariant *) data;
	if(childVariantIndexes.contains(dataVariant))
	{
		info->Member(childVariantIndexes[dataVariant])->SetValue(object, dataVariant->GetVariantValue());
	}
}

void QtPropertyDataIntrospection::ChildNeedUpdate()
{
	for(int i = 0; i < info->MembersCount(); ++i)
	{
		QtPropertyDataDavaVariant *childData = childVariantIndexes.key(i);
		if(NULL != childData)
		{
			DAVA::VariantType childCurValue = info->Member(i)->Value(object);

			if(childCurValue != childData->GetVariantValue())
			{
				childData->SetVariantValue(childCurValue);
			}
		}
	}
}
