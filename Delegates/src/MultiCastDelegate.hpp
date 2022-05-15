#pragma once
#include "Delegates.hpp"
#include <unordered_map>
#include <functional>

namespace DelegateSystem
{
    template <typename R, typename... Params>
    class MultiCastDelegate;

    template <typename R, typename... Params>
    class MultiCastDelegateBase : IDelegate<R, Params...>
    {
        using rIDelegate = IDelegate<R, Params...>;
        using GlobalFunc = R(*)(Params...);
        template<typename T>
        using MemberFunc = R(T::*)(Params...);
    public:

        MultiCastDelegateBase();

        /**
         * Adds the pointer to the member function
         * @param func The function to add from the register
         */
        template<class T>
        void Add(MemberFunc<T> func, T& caller);

        /**
         * Adds the given pointer to member function
         * @param func The function to add to the register
         */
        void Add(GlobalFunc globalFunc);

        /**
         * Removes the given pointer to member function
         * @param func The function to remove from the register
         */
        template<class T>
        void Remove(MemberFunc<T> func, T& caller);

        /**
         * Removes the given pointer to global function
         * @param func The function to remove from the register
         */
        void Remove(GlobalFunc func);

        virtual R Invoke(Params... args) = 0;

        bool operator ==(const MultiCastDelegateBase<R, Params...>& other) const;

        bool operator !=(const MultiCastDelegateBase<R, Params...>& other) const;

        void Clear();

        int GetMethodsN() const;

        int GetMemberMethodsN() const;
        int GetGlobalMethodsN() const;

        virtual ~MultiCastDelegateBase() {}
    protected:

        /**
         * holds an array of func
         */
        struct MemberFuncs
        {
        public:
            struct Func
            {
                //interface function pointer
                std::unique_ptr<rIDelegate> iDelegate;

                //number of times the same function has been registered
                int amount;

                Func() {}
                Func(rIDelegate*& del) : iDelegate(std::unique_ptr<rIDelegate>(del)), amount(1) {}
            };
            //address of functions adress and delegates
            std::unordered_map<void*, Func> functions;
        };

        //global funcs (key 1: address of function, key 2: function)
        std::unordered_map< void*, std::vector<std::unique_ptr<rIDelegate>> > globalFuncs;
        //member funcs (key 1: address of callee; key 2: member function)
        std::unordered_map< void*, MemberFuncs > memberFuncs;

        typedef std::function<void(rIDelegate*)> ForeachFunc;

        void ForEachFunc(ForeachFunc lambda);
    };

    template <typename R, typename... Params>
    class MultiCastDelegate : public MultiCastDelegateBase<R, Params...>
    {
    public:
        using rIDelegate = IDelegate<R, Params...>;
        using rMultiCastDelegateBase = MultiCastDelegateBase<R, Params...>;

        MultiCastDelegate() : rMultiCastDelegateBase() {}

        /**
         * Calls the registered functions in the delegate
         * @param args parameters of the array of functions
         */
        virtual R Invoke(Params... args) override;
    };

    template <typename... Params>
    class MultiCastDelegate<void, Params...> : public MultiCastDelegateBase<void, Params...>
    {
    public:
        using rIDelegate = IDelegate<void, Params...>;
        using rMultiCastDelegateBase = MultiCastDelegateBase<void, Params...>;

        MultiCastDelegate() : rMultiCastDelegateBase() {}

        /**
         * Calls the registered functions in the delegate
         * @param args parameters of the array of functions
         */
        virtual void Invoke(Params... args) override;
    };


    //defining base multicast delegate

    //constructor
    template <typename R, typename... Params>
    MultiCastDelegateBase<R, Params...>::MultiCastDelegateBase() : rIDelegate() {}

    template <typename R, typename... Params> template <class T>
    void MultiCastDelegateBase<R, Params...>::Add(MemberFunc<T> func, T& caller)
    {
        //for more readability
        using std::pair;
        using std::unique_ptr;
        using std::vector;

        using rDelegate = Delegate<T, R, Params...>;

        //takes the address of the caller and function
        void* addressCaller = &caller;
        void* addressFunc = Converter::ForceToVoid<R(T::*)(Params...)>(func);

        //create a delegate to hold the pointer and caller and convert it to a delegate interface
        //the multicast delegate doesn't have to remember the caller so it must be reinterpreted to an interface delegate so that the method call can still happen
        rDelegate* funcHolder = new rDelegate(func, caller);
        rIDelegate* iDelegate = reinterpret_cast<rIDelegate*>(funcHolder);


        if (memberFuncs.count(addressCaller))
        {
            //if the function is already present then add only the interface delegate
            if (memberFuncs[addressCaller].functions.count(addressFunc))
            {
                memberFuncs[addressCaller].functions[addressFunc].amount++;
            }
            else
            {
                //insert the new function address
                memberFuncs[addressCaller].functions.insert(pair< void*, MemberFuncs::Func>(addressFunc, MemberFuncs::Func(iDelegate)));
            }
        }
        else
        {
            //insert callers' address
            memberFuncs.insert(pair<void*, MemberFuncs>(addressCaller, MemberFuncs()));

            //insert function address
            //insert function content (delegate interface)
            memberFuncs[addressCaller].functions.insert(pair< void*, MemberFuncs::Func>(addressFunc, MemberFuncs::Func(iDelegate)));
        }
    }

    template<typename R, typename... Params>
    void MultiCastDelegateBase<R, Params...>::Add(GlobalFunc globalFunc)
    {
        using std::pair;
        using std::unique_ptr;
        using std::vector;
        using DelegateGlobalFunc = Delegate<void, R, Params...>;

        void* adress = Converter::ForceToVoid(globalFunc);

        DelegateGlobalFunc* funcHolder = new DelegateGlobalFunc(globalFunc);
        rIDelegate* iDelegate = reinterpret_cast<rIDelegate*>(funcHolder);

        if (globalFuncs.count(adress))
        {
            globalFuncs[adress].push_back(move(unique_ptr <rIDelegate>(iDelegate)));
        }
        else
        {
            globalFuncs.insert(pair< void*, vector<unique_ptr<rIDelegate>> >(adress, 0));
            globalFuncs[adress].push_back(std::move(unique_ptr <rIDelegate>(iDelegate)));
        }
    }

    template<typename R, typename... Params> template<class T>
    void MultiCastDelegateBase<R, Params...>::Remove(MemberFunc<T> func, T& caller)
    {
        void* funcAdress = Converter::ForceToVoid<R(T::*)(Params...)>(func);
        void* callerAdress = &caller;

        if (!memberFuncs.count(callerAdress)) return;
        if (!memberFuncs[callerAdress].functions.count(funcAdress)) return;

        memberFuncs[callerAdress].functions[funcAdress].amount--;

        if (memberFuncs[callerAdress].functions[funcAdress].amount <= 0) 
            memberFuncs[callerAdress].functions.erase(funcAdress);
    }

    template<typename R, typename... Params>
    void MultiCastDelegateBase<R, Params...>::Remove(GlobalFunc func)
    {
        void* funcAddress = Converter::ForceToVoid<R(*)(Params...)>(func);

        if (!globalFuncs.count(funcAddress)) return;

        globalFuncs[funcAddress].erase(globalFuncs[funcAddress].begin());
    }

    template<typename R, typename... Params>
    void MultiCastDelegateBase<R, Params...>::ForEachFunc(ForeachFunc lambda)
    {
        for (const auto& i : globalFuncs)
        {
            for (const auto& j : i.second)
            {
                lambda(j.get());
            }
        }

        for (const auto& i : memberFuncs)
        {
            for (const auto& j : i.second.functions)
            {
                for (int k = 0; k < j.second.amount; ++k)
                {
                    lambda(j.second.iDelegate.get());
                }
            }
        }
    }

    template<typename R, typename... Params>
    bool MultiCastDelegateBase<R, Params...>::operator ==(const MultiCastDelegateBase<R, Params...>& other) const
    {
        if (globalFuncs.size() != other.globalFuncs.size()) return false;
        if (memberFuncs.size() != other.memberFuncs.size()) return false;
        //checking member functions
        for (const auto& memberFunc : memberFuncs)
        {
            //check if the other doesn't have as a caller a member function
            if (!other.memberFuncs.count(memberFunc.first)) return false;
            //if it does have one then check the size
            if (other.memberFuncs.at(memberFunc.first).functions.size() != memberFunc.second.functions.size()) return false;

            for (const auto& func : memberFunc.second.functions)
            {
                if (!other.memberFuncs.at(memberFunc.first).functions.count(func.first)) return false;
                if (other.memberFuncs.at(memberFunc.first).functions.at(func.first).amount != func.second.amount) return false;
            }
        }

        for (const auto& func : globalFuncs)
        {
            if (!other.globalFuncs.count(func.first)) return false;
        }

        return true;
    }

    template<typename R, typename... Params>
    bool MultiCastDelegateBase<R, Params...>::operator !=(const MultiCastDelegateBase<R, Params...>& other) const
    {
        return !(*this == other);
    }


    template<typename R, typename... Params>
    void MultiCastDelegateBase<R, Params...>::Clear()
    {
        memberFuncs.clear();
        globalFuncs.clear();
    }

    template<typename R, typename... Params>
    int MultiCastDelegateBase<R, Params...>::GetMethodsN() const
    {
        return GetMemberMethodsN() + GetGlobalMethodsN();
    }

    template<typename R, typename... Params>
    int MultiCastDelegateBase<R, Params...>::GetMemberMethodsN() const
    {
        int n = 0;

        for (const auto& mFunc : memberFuncs)
        {
            n += mFunc.second.functions.size();
        }
        return n;
    }

    template<typename R, typename... Params>
    int MultiCastDelegateBase<R, Params...>::GetGlobalMethodsN() const
    {
        int n = 0;

        for (const auto& gFunc : globalFuncs)
        {
            n += gFunc.second.size();
        }
        return n;
    }


    template<typename R, typename... Params>
    R MultiCastDelegate<R, Params...>::Invoke(Params... args)
    {
        R result;
        rMultiCastDelegateBase::ForEachFunc
        (
            [&](rIDelegate* iDelegate)
            {
                result = iDelegate->Invoke(args...);
            }
        );
        return result;
    }

    template<typename... Params>
    void MultiCastDelegate<void, Params...>::Invoke(Params... args)
    {
        rMultiCastDelegateBase::ForEachFunc
        (
            [&](rIDelegate* iDelegate)
            {
                iDelegate->Invoke(args...);
            }
        );
    }
}