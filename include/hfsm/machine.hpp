// HFSM (hierarchical state machine for games and interactive applications)
// Created by Andrew Gresyk
//
// Licensed under the MIT License;
// you may not use this file except in compliance with the License.
//
//
// MIT License
//
// Copyright (c) 2017
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <assert.h>
#include <string.h>

#include <limits>
#include <typeindex>
#include <utility>

#include "detail/array.hpp"
#include "detail/hash_table.hpp"
#include "detail/type_info.hpp"

//------------------------------------------------------------------------------

#ifdef HFSM_ENABLE_STRUCTURE_REPORT
	#define HFSM_IF_STRUCTURE(...)	__VA_ARGS__
#else
	#define HFSM_IF_STRUCTURE(...)
#endif

#ifdef HFSM_ENABLE_LOG_INTERFACE
	#define HFSM_IF_LOGGER(...)		__VA_ARGS__
	#define HFSM_LOGGER_OR(y, n)	y
#else
	#define HFSM_IF_LOGGER(...)
	#define HFSM_LOGGER_OR(y, n)	n
#endif

namespace hfsm {

//------------------------------------------------------------------------------

#ifdef HFSM_ENABLE_STRUCTURE_REPORT
struct StructureEntry {
	bool isActive;
	const wchar_t* prefix;
	const char* name;
};
using MachineStructure = detail::ArrayView<StructureEntry>;
using MachineActivity  = detail::ArrayView<char>;
#endif

#ifdef HFSM_ENABLE_LOG_INTERFACE
struct LoggerInterface {
	enum class Method {
		Substitute,
		Enter,
		Update,
		Transition,
		React,
		Leave,
	};

	virtual void record(const std::type_index& state,
						const char* const stateName,
						const Method method,
						const char* const methodName) = 0;
};

static const char* methodName(const LoggerInterface::Method method) {
	switch (method) {
		case LoggerInterface::Method::Substitute:	return "substitute";
		case LoggerInterface::Method::Enter:		return "enter";
		case LoggerInterface::Method::Update:		return "update";
		case LoggerInterface::Method::Transition:	return "transition";
		case LoggerInterface::Method::React:		return "react";
		case LoggerInterface::Method::Leave:		return "leave";

		default:
			HSFM_BREAK();
			return nullptr;
	}
}
#else
using LoggerInterface = void;
#endif

////////////////////////////////////////////////////////////////////////////////

template <typename TContext, unsigned TMaxSubstitutions = 4>
class M {
	using TypeInfo = detail::TypeInfo;

	template <typename T, unsigned TCapacity>
	using Array = detail::Array<T, TCapacity>;

	template <typename T>
	using ArrayView = detail::ArrayView<T>;

	template <typename TKey, typename TValue, unsigned TCapacity, typename THasher = std::hash<TKey>>
	using HashTable = detail::HashTable<TKey, TValue, TCapacity, THasher>;

	//----------------------------------------------------------------------

#pragma region Utility

public:
	using Context = TContext;
	class Control;

private:
	using Index = unsigned char;
	enum : Index { INVALID_INDEX = std::numeric_limits<Index>::max() };

	enum : unsigned { MaxSubstitutions = TMaxSubstitutions };

	//----------------------------------------------------------------------

	struct Parent {
		#pragma pack(push, 1)
			Index fork  = INVALID_INDEX;
			Index prong = INVALID_INDEX;
		#pragma pack(pop)

		HSFM_IF_DEBUG(TypeInfo forkType);
		HSFM_IF_DEBUG(TypeInfo prongType);

		inline Parent() = default;

		inline Parent(const Index fork_,
					  const Index prong_
					  HSFM_IF_DEBUG(, const TypeInfo forkType_)
					  HSFM_IF_DEBUG(, const TypeInfo prongType_))
			: fork(fork_)
			, prong(prong_)
			HSFM_IF_DEBUG(, forkType(forkType_))
			HSFM_IF_DEBUG(, prongType(prongType_))
		{}

		inline explicit operator bool() const { return fork != INVALID_INDEX && prong != INVALID_INDEX; }
	};

	using Parents = ArrayView<Parent>;

	//----------------------------------------------------------------------

	struct StateRegistry {
	public:
		virtual unsigned add(const TypeInfo stateType) = 0;
	};

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	template <unsigned TCapacity>
	class StateRegistryT
		: public StateRegistry
	{
		enum : unsigned { Capacity = TCapacity };

		using TypeToIndex = HashTable<TypeInfo::Native, unsigned, Capacity>;

	public:
		inline unsigned operator[] (const TypeInfo stateType) const { return *_typeToIndex.find(*stateType); }

		virtual unsigned add(const TypeInfo stateType) override;

	private:
		TypeToIndex _typeToIndex;
	};

	//----------------------------------------------------------------------

	struct Fork {
		#pragma pack(push, 1)
			Index self		= INVALID_INDEX;
			Index active	= INVALID_INDEX;
			Index resumable = INVALID_INDEX;
			Index requested = INVALID_INDEX;
		#pragma pack(pop)

		HSFM_IF_DEBUG(const TypeInfo type);
		HSFM_IF_ASSERT(TypeInfo activeType);
		HSFM_IF_ASSERT(TypeInfo resumableType);
		HSFM_IF_ASSERT(TypeInfo requestedType);

		Fork(const Index index, const TypeInfo type_);
	};
	using ForkPointers = ArrayView<Fork*>;

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	template <typename T>
	struct ForkT
		: Fork
	{
		ForkT(const Index index,
			  const Parent parent,
			  Parents& forkParents)
			: Fork(index, TypeInfo::get<T>())
		{
			forkParents[index] = parent;
		}
	};

	//----------------------------------------------------------------------

	struct Transition {
		enum Type {
			Remain,
			Restart,
			Resume,
			Schedule,

			COUNT
		};

		Type type = Restart;
		TypeInfo stateType;

		inline Transition() = default;

		inline Transition(const Type type_, const TypeInfo stateType_)
			: type(type_)
			, stateType(stateType_)
		{
			assert(type_ < Type::COUNT);
		}
	};
	using TransitionQueue = ArrayView<Transition>;

	//----------------------------------------------------------------------

	template <typename>
	struct _S;

	template <typename, typename...>
	struct _C;

	template <typename, typename...>
	struct _O;

	template <typename>
	class _R;

	//----------------------------------------------------------------------

	template <typename... TS>
	struct WrapState;

	template <typename T>
	struct WrapState<T> {
		using Type = _S<T>;
	};

	template <typename T>
	struct WrapState<_S<T>> {
		using Type = _S<T>;
	};

	template <typename T, typename... TS>
	struct WrapState<_C<T, TS...>> {
		using Type = _C<T, TS...>;
	};

	template <typename T, typename... TS>
	struct WrapState<_O<T, TS...>> {
		using Type = _O<T, TS...>;
	};

	//----------------------------------------------------------------------

#ifdef HFSM_ENABLE_STRUCTURE_REPORT
	struct StateInfo {
		enum RegionType {
			Composite,
			Orthogonal,
		};

		StateInfo() {}

		StateInfo(const unsigned parent_,
				  const RegionType region_,
				  const unsigned depth_,
				  const char* const name_)
			: parent(parent_)
			, region(region_)
			, depth(depth_)
			, name(name_)
		{}

		unsigned parent;
		RegionType region;
		unsigned depth;
		const char* name;
	};
	using StateInfos = detail::ArrayView<StateInfo>;

	using StateStructure = detail::ArrayView<StructureEntry>;
#endif

#pragma endregion

	//----------------------------------------------------------------------

#pragma region Injections

public:

	class Bare {
		template <typename...>
		friend struct _B;

	protected:
		using Control	 = typename M::Control;
		using Context	 = typename M::Context;
		using Transition = typename M::Transition;

	public:
		inline void preSubstitute(Context&)				{}
		inline void preEnter(Context&)					{}
		inline void preUpdate(Context&)					{}
		inline void preTransition(Context&)				{}
		template <typename TEvent>
		inline void preReact(const TEvent&, Context&)	{}
		inline void postLeave(Context&)					{}
	};

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
private:

	template <typename...>
	struct _B;

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	template <typename TInjection, typename... TRest>
	struct _B<TInjection, TRest...>
		: public TInjection
		, public _B<TRest...>
	{
		inline void widePreSubstitute(Context& context);
		inline void widePreEnter(Context& context);
		inline void widePreUpdate(Context& context);
		inline void widePreTransition(Context& context);
		template <typename TEvent>
		inline void widePreReact(const TEvent& event, Context& context);
		inline void widePostLeave(Context& context);
	};

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	template <typename TInjection>
	struct _B<TInjection>
		: public TInjection
	{
		inline void substitute(Control&, Context&)				{}
		inline void enter(Context&)								{}
		inline void update(Context&)							{}
		inline void transition(Control&, Context&)				{}
		template <typename TEvent>
		inline void react(const TEvent&, Control&, Context&)	{}
		inline void leave(Context&)								{}

		inline void widePreSubstitute(Context& context);
		inline void widePreEnter(Context& context);
		inline void widePreUpdate(Context& context);
		inline void widePreTransition(Context& context);
		template <typename TEvent>
		inline void widePreReact(const TEvent& event, Context& context);
		inline void widePostLeave(Context& context);
	};

#pragma endregion

	//----------------------------------------------------------------------

#pragma region Root

	template <typename TApex>
	class _R final {
		using Apex = typename WrapState<TApex>::Type;

	public:
		enum : unsigned {
			ReverseDepth  = Apex::ReverseDepth,
			DeepWidth	  = Apex::DeepWidth,
			StateCount	  = Apex::StateCount,
			ForkCount	  = Apex::ForkCount,
			ProngCount	  = Apex::ProngCount,
			Width		  = Apex::Width,
		};
		static_assert(StateCount < std::numeric_limits<Index>::max(), "Too many states in the hierarchy. Change 'Index' type.");

	private:
		enum : unsigned {
			StateCapacity = (unsigned) 1.3 * Apex::StateCount,
		};

		using StateRegistryImpl		 = StateRegistryT<StateCapacity>;
		using StateParentStorage	 = Array<Parent, StateCount>;
		using ForkParentStorage		 = Array<Parent, ForkCount>;
		using ForkPointerStorage	 = Array<Fork*, ForkCount>;
		using TransitionQueueStorage = Array<Transition, ForkCount>;

	#ifdef HFSM_ENABLE_STRUCTURE_REPORT
		enum : unsigned {
			NameCount	  = Apex::NameCount,
		};

		using Prefix				 = detail::StaticArray<wchar_t, ReverseDepth * 2 + 2>;
		using Prefixes				 = detail::StaticArray<Prefix, StateCount>;

		using StateInfoStorage		 = detail::Array<StateInfo, StateCount>;

		using StructureStorage		 = detail::Array<StructureEntry, NameCount>;
		using ActivityHistoryStorage = detail::Array<char, NameCount>;
	#endif

	public:
		_R(Context& context
		   HFSM_IF_LOGGER(, LoggerInterface* const logger = nullptr));

		~_R();

		void update();

		template <typename TEvent>
		inline void react(const TEvent& event);

		template <typename T>
		inline void changeTo()	{ _requests << Transition(Transition::Type::Restart,  TypeInfo::get<T>());	}

		template <typename T>
		inline void resume()	{ _requests << Transition(Transition::Type::Resume,   TypeInfo::get<T>());	}

		template <typename T>
		inline void schedule()	{ _requests << Transition(Transition::Type::Schedule, TypeInfo::get<T>());	}

		template <typename T>
		inline bool isActive();

		template <typename T>
		inline bool isResumable();

	#ifdef HFSM_ENABLE_STRUCTURE_REPORT
		const MachineStructure& structure() const								{ return _structure;		};
		const MachineActivity&  activity()  const								{ return _activityHistory;	};
	#endif

	#ifdef HFSM_ENABLE_LOG_INTERFACE
		void attachLogger(LoggerInterface* const logger)						{ _logger = logger;			}
	#endif

	protected:
		void processTransitions();
		void requestImmediate(const Transition request);
		void requestScheduled(const Transition request);

		inline unsigned id(const Transition request) const	{ return _stateRegistry[*request.stateType];	}

	#ifdef HFSM_ENABLE_STRUCTURE_REPORT
		void getStateNames();
		void udpateActivity();
	#endif

	private:
		Context& _context;

		StateRegistryImpl _stateRegistry;

		StateParentStorage _stateParents;
		ForkParentStorage  _forkParents;
		ForkPointerStorage _forkPointers;

		TransitionQueueStorage _requests;

		Apex _apex;

	#ifdef HFSM_ENABLE_STRUCTURE_REPORT
		Prefixes _prefixes;
		StateInfoStorage _stateInfos;

		StructureStorage _structure;
		ActivityHistoryStorage _activityHistory;

		struct DebugTransitionInfo {
			typename Transition::Type type;
			TypeInfo state;

			enum Source {
				Update,
				Substitute,
				Linger,

				COUNT
			};
			Source source;

			inline DebugTransitionInfo() = default;

			inline DebugTransitionInfo(const Transition transition,
									   const Source source_)
				: type(transition.type)
				, state(transition.stateType)
				, source(source_)
			{
				assert(source_ < Source::COUNT);
			}
		};
		using DebugTransitionInfos = Array<DebugTransitionInfo, 2 * ForkCount>;
		DebugTransitionInfos _lastTransitions;
	#endif

		HFSM_IF_LOGGER(LoggerInterface* _logger);
	};

	//----------------------------------------------------------------------

public:

	class Control final {
		template <typename>
		friend class _R;

	private:
		Control(TransitionQueue& requests)
			: _requests(requests)
		{}

	public:
		template <typename T>
		inline void changeTo()	{ _requests << Transition(Transition::Type::Restart,  TypeInfo::get<T>());	}

		template <typename T>
		inline void resume()	{ _requests << Transition(Transition::Type::Resume,	  TypeInfo::get<T>());	}

		template <typename T>
		inline void schedule()	{ _requests << Transition(Transition::Type::Schedule, TypeInfo::get<T>());	}

		inline unsigned requestCount() const									{ return _requests.count();	}

	private:
		TransitionQueue& _requests;
	};

#pragma endregion

	//----------------------------------------------------------------------

#pragma region Public Typedefs

	using Base = _B<Bare>;

	template <typename... TInjections>
	using BaseT = _B<TInjections...>;

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	template <typename TState, typename... TSubStates>
	using Composite	= _C<TState, TSubStates...>;

	template <typename... TSubStates>
	using CompositePeers = _C<Base, TSubStates...>;

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	template <typename TState, typename... TSubStates>
	using Orthogonal = _O<TState, TSubStates...>;

	template <typename... TSubStates>
	using OrthogonalPeers = _O<Base, TSubStates...>;

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	template <typename TState, typename... TSubStates>
	using Root = _R<Composite<TState, TSubStates...>>;

	template <typename... TSubStates>
	using PeerRoot = _R<CompositePeers<TSubStates...>>;

	template <typename TState, typename... TSubStates>
	using OrthogonalRoot = _R<Orthogonal<TState, TSubStates...>>;

	template <typename... TSubStates>
	using OrthogonalPeerRoot = _R<OrthogonalPeers<TSubStates...>>;

	//----------------------------------------------------------------------

#pragma endregion
};

template <typename TContext, unsigned TMaxSubstitutions = 4>
using Machine = M<TContext, TMaxSubstitutions>;

////////////////////////////////////////////////////////////////////////////////

}

#include "detail/machine.inl"

#undef HFSM_IF_STRUCTURE
#undef HFSM_IF_LOGGER
#undef HFSM_LOGGER_OR
