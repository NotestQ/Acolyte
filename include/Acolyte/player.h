#pragma once
#include <Vagante/entities.h>
#include <Acolyte/exports.h>
#include <sigslot/signal.hpp>

namespace sdk::Player {
	using UpdateStatsFn = void(__fastcall*)(::Player* playerThis, void* edx, bool param_1, bool clientOverride);
	using AddNewSkillFn = void(__fastcall*)(::Player* playerThis, void* edx, Skill* skillptr, bool param_2);
	using SetCurrSkillFn = void(__fastcall*)(::Player* playerThis, void* edx, uint skillIndex, bool param_2);
	using SkillConstructorFn = Skill*(__fastcall*)(Skill* skillThis, void* edx, SkillType skillType);

	ACOLYTE_API extern UpdateStatsFn UpdateStats;

	ACOLYTE_API extern AddNewSkillFn AddNewSkill;
	ACOLYTE_API extern SetCurrSkillFn SetCurrSkill;
	ACOLYTE_API extern SkillConstructorFn SkillConstructor;

	ACOLYTE_API extern sigslot::signal<std::shared_ptr<::Player>&> OnPlayerAdded;
	ACOLYTE_API extern sigslot::signal<::Player&> OnLevelSet;
}