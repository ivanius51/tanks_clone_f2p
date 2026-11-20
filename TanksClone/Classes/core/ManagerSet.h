#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <tuple>

#include "core/TypeList.h"

namespace tanks
{

template <typename TList>
class ManagerSet;

/**
 * Owns one instance of each type in the list: Create() builds them in list order, Destroy() (or the destructor)
 * removes them in reverse order, so a manager may use the ones listed before it for its whole life.
 */
template <typename... TManagers>
class ManagerSet<TypeList<TManagers...>>
{
public:
	using List = TypeList<TManagers...>;

	ManagerSet() = default;
	ManagerSet(const ManagerSet&) = delete;
	ManagerSet& operator=(const ManagerSet&) = delete;

	~ManagerSet()
	{
		Destroy();
	}

	void Create()
	{
		if ( bCreated )
		{
			return;
		}
		(CreateOne<TManagers>(), ...);
		bCreated = true;
	}

	void Destroy()
	{
		if ( !bCreated )
		{
			return;
		}
		bCreated = false;
		DestroyFirst<sizeof...(TManagers)>();
	}

	bool IsCreated() const
	{
		return bCreated;
	}

	template <typename TManager>
	TManager& Get() const
	{
		static_assert(IsInTypeList<TManager, List>::value, "the type is not in the manager list");
		const std::unique_ptr<TManager>& Manager = std::get<TypeIndexOf<TManager, List>::value>(Managers);
		assert(Manager && "managers are used only between Create() and Destroy()");
		return *Manager;
	}

private:
	template <typename TManager>
	void CreateOne()
	{
		std::get<TypeIndexOf<TManager, List>::value>(Managers) = std::make_unique<TManager>();
	}

	template <std::size_t TCount>
	void DestroyFirst()
	{
		if constexpr ( TCount > 0 )
		{
			std::get<TCount - 1>(Managers).reset();
			DestroyFirst<TCount - 1>();
		}
	}

	std::tuple<std::unique_ptr<TManagers>...> Managers;
	bool bCreated = false;
};

} // namespace tanks
