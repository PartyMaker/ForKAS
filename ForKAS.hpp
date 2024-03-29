#pragma once
#include <string>
#include <map>
#include <unordered_map>
#include <atomic>
#include <assert.h>
#include <memory>
#include <sstream>
#include <algorithm>
#include <mutex>
//-------------------------------------------------------------------------------------------------------------
struct product
{
	product(
		const std::string& _id,
		const std::string& _manufacturer,
		const std::string& _name,
		uint64_t           _cost) : id(_id), manufacturer(_manufacturer), name(_name), cost(_cost) {}
	std::string id;
	std::string manufacturer;
	std::string name;
	uint64_t    cost;
};
//-------------------------------------------------------------------------------------------------------------
template<typename T>
struct key
{
	mutable const T* p;

	key(const T* v = 0) : p(v) {}
	bool operator< (const key& x) const { return *p < *x.p; }
	bool operator==(const key<std::string>& v) const { return *v.p == *p; }
};
struct Hash
{
	std::size_t operator()(const key<std::string>& v) const {
		return std::hash<std::string>()(*v.p);
	}
};
//-------------------------------------------------------------------------------------------------------------
template<typename I>
struct map_iterator_adapter: I
{
	typedef const typename I::value_type::second_type value_type;
	typedef value_type* pointer;
	typedef value_type& reference;

	map_iterator_adapter() = default;
	map_iterator_adapter(I i): I(i) {}

	reference operator*  () const { return  I::operator* ().second;   }
	pointer   operator-> () const { return& I::operator-> ()->second; }
};
//-------------------------------------------------------------------------------------------------------------
struct product_set
{
	typedef std::unordered_map<key<std::string>, product, Hash> id_map;
	typedef map_iterator_adapter<id_map::const_iterator> iterator;
	typedef std::multimap<key<std::string>, iterator> name_map;
	auto insert(const product& v)
	{
		std::lock_guard<std::mutex> lock_(m_mutex);
		{
			 auto i(m_name_map.find(&v.id));
			 if (i != m_name_map.end())
				return std::make_pair(i->second, false);
		}
		auto r(m_id_map.insert(id_map::value_type(&v.id, v)));
		iterator i(r.first);
		if (r.second)
		{
			r.first->first.p = &i->id;
			m_name_map.insert(name_map::value_type(&i->manufacturer, i));
		}
		return std::make_pair(i, r.second);
	}
	auto find_name(const std::string& name) const
	{
		std::lock_guard<std::mutex> lock_(m_mutex);
		return m_name_map.equal_range(&name);
	}
	auto erase(const std::string& id)
	{
		std::lock_guard<std::mutex> lock_(m_mutex);
		auto i(m_id_map.find(&id));
		if (i != m_id_map.end())
		{
			auto a = m_name_map.equal_range(&i->second.manufacturer);
			for (auto it = a.first; it != a.second; it++)
				if (it->second == i)
				{
					m_name_map.erase(it);
					break;
				}
			return m_id_map.erase(i);
		}
		return m_id_map.end();
	}
	iterator find_id(const std::string& id) const
	{ 
		std::lock_guard<std::mutex> lock_(m_mutex);
		return m_id_map.find(&id);
	}
	iterator begin() const
	{ 
		std::lock_guard<std::mutex> lock_(m_mutex);
		return m_id_map.begin();
	}
	iterator end() const
	{
		std::lock_guard<std::mutex> lock_(m_mutex);
		return m_id_map.end(); 
	}

private:
	id_map m_id_map;
	name_map  m_name_map;
	mutable std::mutex m_mutex;
};
//-------------------------------------------------------------------------------------------------------------
class CSVRow
{
public:
	std::string const& operator[](std::size_t index) const { return m_data[index]; }
	std::size_t size() const { return m_data.size(); }
	void readNextRow(std::istream& str)
	{
		std::string         line;
		std::getline(str, line);

		std::stringstream   lineStream(line);
		std::string         cell;

		m_data.clear();
		while (std::getline(lineStream, cell, ','))
		{
			m_data.push_back(cell);
		}
		// This checks for a trailing comma with no data after it.
		if (!lineStream && cell.empty())
		{
			// If there was a trailing comma then add an empty element.
			m_data.push_back("");
		}
	}
private:
	std::vector<std::string> m_data;
};
//-------------------------------------------------------------------------------------------------------------
