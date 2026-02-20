#include <Vagante/items.h>
#include <Vagante/entities.h>
#include <Acolyte/logger.h>
#include <Acolyte/utils.h>
#include <Acolyte/item.h>
#include <sdk_registry.h>

static Logger playerLogger("Acolyte");
static const uintptr_t moduleBaseAddress = (uintptr_t)GetModuleHandleA(NULL);

sdk::Item::WeaponConstructorFn pOriginalWeaponConstructor = nullptr;
Weapon* __fastcall weaponConstructorDetour(Weapon* weaponThis, void*, std::string* attackScript, std::string* weaponProperties) {
//	playerLogger.LogDebug((std::string)"Constructing weapon with attack script \"" + *attackScript + "\"\
// and weapon properties \"" + *weaponProperties + "\"");
	return pOriginalWeaponConstructor(weaponThis, NULL, attackScript, weaponProperties);
}

sdk::Item::EquipWeaponFn pOriginalEquipWeapon = nullptr;
void __fastcall equipWeaponDetour(::Player* playerThis, void*, std::shared_ptr<Weapon> weaponShrdPtr, std::shared_ptr<::Item> itemShrdPtr) {
//	std::string attackScriptName = weaponShrdPtr.get()->attackScript->attackScriptName;
//	std::string weaponName = weaponShrdPtr.get()->weaponProperties->weaponName;
//
//	playerLogger.LogDebug((std::string)"Equipping weapon with attack script \"" + attackScriptName + "\"\
// and weapon properties \"" + weaponName + "\"");
	return pOriginalEquipWeapon(playerThis, NULL, weaponShrdPtr, itemShrdPtr);
}

namespace sdk::Item {
	WeaponSetLevelFn WeaponSetLevel = nullptr;
	WeaponConstructorFn WeaponConstructor = nullptr;
	EquipWeaponFn EquipWeapon = nullptr;

	void InitItem() {
		playerLogger.LogDebug("Initializing Item system...");

		WeaponConstructor = reinterpret_cast<WeaponConstructorFn>(moduleBaseAddress + 0x3EEA10);
		WeaponSetLevel = reinterpret_cast<WeaponSetLevelFn>(moduleBaseAddress + 0x3EF770);
		EquipWeapon = reinterpret_cast<EquipWeaponFn>(moduleBaseAddress + 0x2FCBF0);

		Utils::CreateHook(reinterpret_cast<void**>(WeaponConstructor), &weaponConstructorDetour, reinterpret_cast<void**>(&pOriginalWeaponConstructor));
		Utils::CreateHook(reinterpret_cast<void**>(EquipWeapon), &equipWeaponDetour, reinterpret_cast<void**>(&pOriginalEquipWeapon));
		
		playerLogger.LogDebug("Item system initialized!");
	}
	SDK_REGISTER(InitItem);
}