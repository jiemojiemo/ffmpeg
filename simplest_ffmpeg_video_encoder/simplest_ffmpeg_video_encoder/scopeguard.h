#pragma once
#include <vector>
#include <functional>

class ScopeGuard{
private:
    bool m_dismissed;
    std::function<void ()> m_onExitScope;
private:
    ScopeGuard( const ScopeGuard& sg );
    ScopeGuard& operator=( const ScopeGuard& sg );
public:
    explicit ScopeGuard( std::function<void()> onExitScope )
        : m_onExitScope( onExitScope ), m_dismissed( false )
        {

        }
	void Dismiss() {
		m_dismissed = true;
	}
    ~ScopeGuard()
    {
        if( !m_dismissed )
        {
            m_onExitScope();
        }
    }

};

#define SCOPEGUARD_LINENAME_CAT(name, line) name##line
#define SCOPEGUARD_LINENAME(name, line) SCOPEGUARD_LINENAME_CAT(name, line)
#define ON_SCOPE_EXIT(callback) ScopeGuard SCOPEGUARD_LINENAME(EXIT,__LINE__)(callback)
