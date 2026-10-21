#pragma once
#include "settings/Profiles.h"
#include <optional>
class QWidget;
namespace terlox { std::optional<Profile> editProfile(QWidget* parent); }
