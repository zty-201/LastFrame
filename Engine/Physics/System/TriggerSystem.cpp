#include "pch.hpp"            // includes ENTt from pch
#include "global.hpp"         // brings in event_dispatcher & registry

#include "TriggerSystem.hpp"
#include "TriggerEvents.hpp"
#include "TransformComponent.h"
#include "TriggerBoxComponent.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

#include <unordered_set>
namespace EngineFramework
{
	// --- compute AABB from world matrix and half extents ---
	static void ComputeAABB(const glm::mat4& world,
		const glm::vec3& half,
		glm::vec3& outMin,
		glm::vec3& outMax)
	{
		glm::vec3 c = glm::vec3(world[3]);
		glm::vec3 x = glm::vec3(world[0]);
		glm::vec3 y = glm::vec3(world[1]);
		glm::vec3 z = glm::vec3(world[2]);

		glm::vec3 ext =
			glm::abs(x) * half.x +
			glm::abs(y) * half.y +
			glm::abs(z) * half.z;

		outMin = c - ext;
		outMax = c + ext;
	}

	static bool Overlap(const glm::vec3& aMin, const glm::vec3& aMax,
		const glm::vec3& bMin, const glm::vec3& bMax)
	{
		return (aMin.x <= bMax.x && aMax.x >= bMin.x) &&
			(aMin.y <= bMax.y && aMax.y >= bMin.y) &&
			(aMin.z <= bMax.z && aMax.z >= bMin.z);
	}

	void TriggerSystem::Update(entt::registry& reg)
	{
		/*
			Checks if the player enters a triggerbox.
			An entity called an InteractionDetector (in its namecomponent) is used for this detection, and is attached to the player.
		*/
		using Trigger_AABB = std::pair<glm::vec3, glm::vec3>;

		//==Get detector AABB
		entt::entity detector_id{};
		//Get detector id
		reg.view<NameComponent>().each([&detector_id](entt::entity entity, const NameComponent& name) {
			if (name.name == "InteractionDetector") detector_id = entity;
			});
		if (detector_id == entt::entity{}) return;
		if (!reg.all_of<TransformComponent>(detector_id)) return;

		//Get detecttor aabb.
		//Either get from existing triggerbox or create one or use collider component.
		Trigger_AABB detector_aabb{};
		TransformComponent& detector_tf = reg.get<TransformComponent>(detector_id);
		if (reg.all_of<TriggerBoxComponent>(detector_id))  //Use trigger cached aabb
		{
			TriggerBoxComponent& detector_box = reg.get<TriggerBoxComponent>(detector_id);
			detector_aabb = GetAABB(detector_box, detector_tf);
		}
		else if (reg.all_of<ColliderComponent>(detector_id))  //Use collider bounds
		{

			//Use top-level collider AABB to form an offset matrix.
			ColliderComponent& detector_collider = reg.get<ColliderComponent>(detector_id);
			const glm::vec3& min = detector_collider.minBounds, & max = detector_collider.maxBounds;
			glm::vec3 center = (min + max) * 0.5f;
			glm::vec3 size = max - min;


			glm::mat4 offset_mtx = glm::translate(glm::mat4(1.0f), center)
				* glm::scale(glm::mat4(1.0f), size);

			//Because collider component min, max bounds are defined relative to parent, need to apply offset to parent to get the full result.
			glm::mat4 world_mtx = detector_tf.modelMatrix * offset_mtx;
			ComputeAABB(world_mtx, glm::vec3(0.5f), detector_aabb.first, detector_aabb.second);
		}
		else
		{
			//Don't bother checking if the detector doesn't have either.
			return;
		}



		//==Iterate over all triggerboxes in scene.
		auto triggers = reg.view<TransformComponent, TriggerBoxComponent>();
		for (auto [other_id, other_tf, other_box] : triggers.each())
		{
			//Check for collision.
			if (!other_box.enabled) continue;
			other_box.is_collided_this_frame = false;
			if (other_id == detector_id) continue;


			/*
				Send out events when
				- player enters
				- player still inside
				- player leaves
			*/
			Trigger_AABB other_aabb = GetAABB(other_box, other_tf);
			if (!Overlap(detector_aabb.first, detector_aabb.second, other_aabb.first, other_aabb.second))
			{
				//Player not inside, check if they were here last frame.
				if (other_box.currentInside.find(detector_id) != other_box.currentInside.end())
				{
					other_box.currentInside.erase(detector_id);
					event_dispatcher.trigger(Event_OnTriggerExit{ other_id, detector_id });
				}
				continue;
			}

			//Player stores this obj id.
			if (reg.all_of<TriggerBoxComponent>(detector_id)) reg.get<TriggerBoxComponent>(detector_id).currentInside.insert(other_id);


			if (other_box.currentInside.find(detector_id) == other_box.currentInside.end())
			{
				event_dispatcher.trigger(Event_OnTriggerEnter{ other_id, detector_id });
			}
			else
			{
				event_dispatcher.trigger(Event_OnTriggerStay{ other_id, detector_id });
			}


			other_box.currentInside.insert(detector_id);
		}
	}


	std::pair<glm::vec3, glm::vec3> TriggerSystem::GetAABB(TriggerBoxComponent& t_b, const TransformComponent& parent_transform)
	{
		bool is_old_aabb_valid = true;
		//Used to compare glm data structs like vector or matrix against each other.
#define GLM_NOT_EQUAL(v1, v2) !glm::all(glm::epsilonEqual(v1, v2, 0.0001f))

		//Check for any changes in its values, and if aabb is initialized yet.
		//TODO: Can be optimized into a sentinel value set when the setter method for TriggerBox is called. Then no need to check if it's been changed.
		if (
			(t_b.old_radius != t_b.radius) ||
			(t_b.old_capHalfHeight != t_b.capHalfHeight) ||
			(t_b.old_capRadius != t_b.capRadius) ||
			GLM_NOT_EQUAL(t_b.old_localPosition, t_b.localPosition) ||
			GLM_NOT_EQUAL(t_b.old_localEulerDeg, t_b.localEulerDeg) ||
			GLM_NOT_EQUAL(t_b.old_halfExtents, t_b.halfExtents) ||
			GLM_NOT_EQUAL(t_b.old_parent_pos, parent_transform.position) ||
			GLM_NOT_EQUAL(t_b.old_parent_rotation, parent_transform.rotation) ||
			GLM_NOT_EQUAL(t_b.old_parent_scale, parent_transform.scale) ||
			!GLM_NOT_EQUAL(t_b.min, t_b.max)
			)
		{
			is_old_aabb_valid = false;
		}
		if (is_old_aabb_valid)
		{
			return { t_b.min, t_b.max };
		}
		//==Have to recalculate aabb.
		//Get AABB world transformation matrix by applying offset to parent's transform.
		glm::mat4 offset =
			glm::translate(glm::mat4(1.0f), t_b.localPosition) *
			glm::mat4_cast(glm::quat(glm::radians(t_b.localEulerDeg))) *
			glm::scale(glm::mat4(1.0f), t_b.halfExtents * 2.0f);

		glm::mat4 world_matrix = parent_transform.modelMatrix * offset;

		//LOGD("Parent Matrix: \n[%.2f, %.2f, %.2f, %.2f]\n"
		//	"[%.2f, %.2f, %.2f, %.2f]\n"
		//	"[%.2f, %.2f, %.2f, %.2f]\n"
		//	"[%.2f, %.2f, %.2f, %.2f]\n",
		//	world_matrix[0][0], world_matrix[1][0], world_matrix[2][0], world_matrix[3][0],
		//	world_matrix[0][1], world_matrix[1][1], world_matrix[2][1], world_matrix[3][1],
		//	world_matrix[0][2], world_matrix[1][2], world_matrix[2][2], world_matrix[3][2],
		//	world_matrix[0][3], world_matrix[1][3], world_matrix[2][3], world_matrix[3][3]);
		ComputeAABB(world_matrix, glm::vec3(0.5f), t_b.min, t_b.max);

		//Assign to indicate no need for recalculation next access.
		t_b.old_localPosition = t_b.localPosition;
		t_b.old_localEulerDeg = t_b.localEulerDeg;
		t_b.old_halfExtents = t_b.halfExtents;
		t_b.old_radius = t_b.radius;
		t_b.old_capHalfHeight = t_b.capHalfHeight;
		t_b.old_capRadius = t_b.capRadius;
		t_b.old_parent_pos = parent_transform.position;
		t_b.old_parent_rotation = parent_transform.rotation;
		t_b.old_parent_scale = parent_transform.scale;
		return { t_b.min, t_b.max };
	}
}