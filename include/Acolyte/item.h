#pragma once
#include <Acolyte/vagante.h>
#include <Acolyte/exports.h>
#include <memory>

namespace sdk::Item {
	using WeaponConstructorFn = Weapon * (__fastcall*)(Weapon* weaponThis, void* edx, std::string* attackScript, std::string* weaponProperties);
	using EquipWeaponFn = void(__fastcall*)(::Player* playerThis, void* edx, std::shared_ptr<Weapon> weaponShrdPtr, std::shared_ptr<::Item> itemShrdPtr);

	ACOLYTE_API extern WeaponConstructorFn WeaponConstructor;
	ACOLYTE_API extern EquipWeaponFn EquipWeapon;
}