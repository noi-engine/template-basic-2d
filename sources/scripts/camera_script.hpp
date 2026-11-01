#pragma once

#include <noi_engine/core/ecs/entity.hpp>
#include <noi_engine/core/model/script.hpp>

namespace noi_engine_game
{
	class camera_script : public noi_engine::script
	{
	public:
		auto on_create(noi_engine::entity self,
					   const noi_engine::script_context &context)
			-> void override;

		explicit camera_script() = default;

		// Registers the script under the name "camera_script" (the name scenes use).
		NOI_ENGINE_SCRIPT(camera_script);
	};
}  // namespace noi_engine_game