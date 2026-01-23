bool sdk_register(void(*fn)());
void InitAllSDKComponents();

#define SDK_REGISTER(fn) \
	static const bool SDK_REGISTRATION_##fn## = sdk_register(&fn);