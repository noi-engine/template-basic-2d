#pragma once

#include <noi_engine/core/ecs/entity.hpp>
#include <noi_engine/core/model/script.hpp>

namespace noi_engine_game
{
	class player_script : public noi_engine::script
	{
	public:
		auto on_update(noi_engine::entity self,
					   const noi_engine::script_context &context, float dt)
			-> void override;

		explicit player_script() = default;
	};
}
