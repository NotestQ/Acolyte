#pragma once
#include <Vagante/entities.h>
#include <Acolyte/exports.h>
#include <sigslot/signal.hpp>

namespace sdk::Player {
	ACOLYTE_API sigslot::signal<std::shared_ptr<::Player>&> OnPlayerAdded;
}