#pragma once
#include <xstddef>
#include <type_traits>

template <class _Ty, typename = std::void_t<>>
struct _Impl_instance : std::false_type {};

template <class _Ty>
struct _Impl_instance<_Ty, std::void_t<decltype(_Ty::Instance())>> : std::true_type {};

template <class _Ty>
using is_instance_class = _Impl_instance<_Ty>;

template <class _Ty>
inline constexpr bool is_instance_class_v = _Impl_instance<_Ty>::value;

template <
	class _Ty,
	std::enable_if_t<!std::is_default_constructible_v<_Ty>, int> = 0>
struct ObjectProxy final
{
	constexpr ObjectProxy() noexcept = default;

	ObjectProxy(const ObjectProxy&) = delete;
	ObjectProxy& operator=(const ObjectProxy&) = delete;
	ObjectProxy(ObjectProxy&&) = delete;
	ObjectProxy& operator=(ObjectProxy&&) = delete;

	template <typename = std::enable_if<is_instance_class_v<_Ty>>>
	constexpr [[nodiscard]] _Ty* operator->() const noexcept
	{
		return std::addressof(_Ty::Instance());
	}
};