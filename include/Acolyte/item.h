#pragma once
#include <Vagante/items.h>
#include <Acolyte/exports.h>
#include <memory>

namespace sdk::Item {
	using WeaponSetLevelFn = void(__fastcall*)(Weapon * weaponThis, void* edx, Level* level);
	using WeaponConstructorFn = Weapon* (__fastcall*)(Weapon* weaponThis, void* edx, std::string* attackScript, std::string* weaponProperties);
	using EquipWeaponFn = void (__fastcall*)(::Player* playerThis, void* edx, std::shared_ptr<Weapon> weaponShrdPtr, std::shared_ptr<::Item> itemShrdPtr);
	
	ACOLYTE_API extern WeaponSetLevelFn WeaponSetLevel;
	ACOLYTE_API extern WeaponConstructorFn WeaponConstructor;
	ACOLYTE_API extern EquipWeaponFn EquipWeapon;
}