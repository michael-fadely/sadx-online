#pragma once

enum class DirtySetMode
{
	/**
	 * \brief Fastest - mark as dirty on any assignment.
	 */
	on_assignment,
	/**
	 * \brief Fast - always compare until marked as dirty.
	 */
	until_dirty
};

template <typename T, DirtySetMode set_mode = DirtySetMode::until_dirty>
class dirty_t
{
protected:
	T    _data {};
	bool _dirty = false;

public:
	dirty_t() = default;

	explicit dirty_t(const T& initial)
	{
		_data = initial;
	}

	bool dirty() const
	{
		return _dirty;
	}

	void clear()
	{
		_dirty = false;
	}

	void mark()
	{
		_dirty = true;
	}

	const T& data() const
	{
		return _data;
	}

	void data(const T& value)
	{
		assign(value);
	}

	dirty_t& operator=(const T& value)
	{
		assign(value);
		return *this;
	}

	operator const T&()
	{
		return data();
	}

protected:
	void assign(const T& value)
	{
		if constexpr (set_mode == DirtySetMode::on_assignment)
		{
			_dirty = true;
		}
		else if constexpr (set_mode == DirtySetMode::until_dirty)
		{
			if (!_dirty)
			{
				_dirty = value != _data;
			}
		}

		_data = value;
	}
};
