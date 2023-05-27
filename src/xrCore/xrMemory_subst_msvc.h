template <typename T, typename... Args>
T* xr_new(Args&&... args) {
	T* ptr = static_cast<T*>(Memory.mem_alloc(sizeof(T)));
	return new (ptr) T(std::forward<Args>(args)...);
}

template <bool _is_pm, typename T>
struct xr_special_free
{
	IC void operator()(T*& ptr)
	{
		void* _real_ptr = dynamic_cast<void*>(ptr);
		ptr->~T();
		Memory.mem_free(_real_ptr);
	}
};

template <typename T>
struct xr_special_free<false, T>
{
	IC void operator()(T*& ptr)
	{
		ptr->~T();
		Memory.mem_free(ptr);
	}
};

template <class T>
IC	void	xr_delete(T*& ptr)
{
	if (ptr)
	{
		xr_special_free<std::is_polymorphic<T>::value, T>()(ptr);
		ptr = NULL;
	}
}
template <class T>
IC	void	xr_delete(T* const& ptr)
{
	if (ptr)
	{
		xr_special_free<std::is_polymorphic<T>::value, T>()(const_cast<T*&>(ptr));
		const_cast<T*&>(ptr) = NULL;
	}
}
