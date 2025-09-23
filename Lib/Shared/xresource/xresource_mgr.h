#ifndef XRESOURCE_MGR_H
#define XRESOURCE_MGR_H
#pragma once

#include <cassert>
#include <unordered_map>
#include <array>
#include "xresource_guid.h"

//----------------------------------------------------------------------------------
// Example on registering a resource type
//----------------------------------------------------------------------------------
// using texture_guid = xresource::def_guid<xresource::type_guid("texture")>;  // First we define the GUID
//
// template<>
// struct xresource::loader< texture_guid.m_Type >                  // Now we specify the loader and we must fill in all the information
// {
//      //--- Expected static parameters ---
//      constexpr static inline auto         type_name_v        = "Texture";            // Name of the type used in the resource path
//      constexpr static inline auto         use_death_march_v  = true;                 // Will wait at least 1 frame before releasing the resource
//      using                                data_type          = xgpu::texture         // This is the actual data type of the runtime resource itself...
//
//      static data_type*                    Load   ( xresource::mgr& Mgr,                    const full_guid& GUID );
//      static void                          Destroy( xresource::mgr& Mgr, data_type& Data,   const full_guid& GUID );
// };
// After you have define the loader type you need to register it, like this...
// inline static xresource::loader_registration<texture_guid.m_Type> UniqueName;
//
// The implementation of the above functions should be done in a cpp file
// This will minimize dependencies and help keep the code clean
//----------------------------------------------------------------------------------
namespace xresource
{
    struct mgr;
    template< type_guid TYPE_GUID_V >
    struct loader {};

    namespace details
    {
        struct registration_base
        {
            inline static registration_base* s_pHead = {nullptr};
            registration_base*               m_pNext;

            registration_base() : m_pNext { s_pHead }
            {
                s_pHead = this;
            }
                                                                      registration_base   ( registration_base&& )                                               = delete;
                                                                      registration_base   ( const registration_base& )                                          = delete;
                            constexpr virtual                        ~registration_base   ( void )                                                              = default;
                                      const registration_base&        operator =          ( const registration_base& )                                          = delete;
                                      const registration_base&        operator =          ( registration_base&& )                                               = delete;
            [[nodiscard]]   constexpr virtual void*                   Load                ( xresource::mgr& Mgr,              const full_guid& GUID ) const     = 0;
                            constexpr virtual void                    Destroy             ( xresource::mgr& Mgr, void* pData, const full_guid& GUID ) const     = 0;
            [[nodiscard]]   constexpr virtual std::wstring_view       getTypeName         ( void )                                                    const     = 0;
            [[nodiscard]]   constexpr virtual type_guid               getTypeGuid         ( void )                                                    const     = 0;
            [[nodiscard]]   constexpr virtual bool                    hasDeathmarchOn     ( void )                                                    const     = 0;
        };

        //template< type_guid TYPE_GUID_V, typename = void > struct get_custom_name                                                                     { static inline           const char* value = []{return typeid(loader<TYPE_GUID_V>::data_type).name(); }(); };
        //template< type_guid TYPE_GUID_V >                  struct get_custom_name< TYPE_GUID_V, std::void_t< typename loader<TYPE_GUID_V>::name_v > > { static inline constexpr const char* value = loader<TYPE_GUID_V>::name_v; };
    }

    //
    // Registering a loader for a resource type
    //
    template< type_guid TYPE_GUID_V >
    struct loader_registration final : details::registration_base
    {
        using loader = loader<TYPE_GUID_V>;
        using type   = typename loader::data_type;

        [[nodiscard]] constexpr void* Load(xresource::mgr& Mgr, const full_guid& GUID) const override
        {
            return loader::Load(Mgr, GUID);
        }

        constexpr void Destroy(xresource::mgr& Mgr, void* pData, const full_guid& GUID) const override
        {
            loader::Destroy(Mgr, std::move(*static_cast<type*>(pData)), GUID);
        }

        [[nodiscard]] constexpr std::wstring_view getTypeName() const override
        {
            return loader::type_name_v;
        }

        [[nodiscard]] constexpr type_guid getTypeGuid() const override
        {
            return TYPE_GUID_V;
        }

        [[nodiscard]] constexpr bool hasDeathmarchOn() const override
        {
            return loader::use_death_march_v;
        }

    };

    //
    // RSC MANAGER
    //
    namespace details
    {
        struct instance_info
        {
            void*                       m_pData     = { nullptr };
            full_guid                   m_Guid      = {};
            int                         m_RefCount  = { 1 };
        };

        struct universal_type
        {
            type_guid                   m_TypeGUID;
            details::registration_base* m_pRegistration;
            std::wstring_view           m_TypeName;
            bool                        m_bUseDeathMarch;
        };
    }

    // Resource Manager
    struct mgr
    {
        ~mgr()
        {
            // If the user have give us ownership of the user data we must free it
            if ( m_bOwnsUserData && m_pUserData )
            {
                delete m_pUserData;
            }
        }

        //-------------------------------------------------------------------------

        void Initiallize( std::size_t MaxResource = 1000 ) noexcept
        {
            m_MaxResources = MaxResource;

            m_InfoBuffer = std::make_unique<details::instance_info[]>(m_MaxResources);

            //
            // Initialize our memory manager of instance infos
            //
            for (int i = 0, end = (int)m_MaxResources - 1; i != end; ++i)
            {
                m_InfoBuffer[i].m_pData = &m_InfoBuffer[i + 1];
            }
            m_InfoBuffer[m_MaxResources - 1].m_pData = nullptr;
            m_pInfoBufferEmptyHead = m_InfoBuffer.get();


            //
            // Insert all the types into the hash table
            //
            int TotalTypes = 0;
            for ( details::registration_base* p = details::registration_base::s_pHead; p ; p = p->m_pNext )
            {
                TotalTypes++;
            }

            m_RegisteredTypes.reserve(TotalTypes);

            for (details::registration_base* p = details::registration_base::s_pHead; p; p = p->m_pNext)
            {
                m_RegisteredTypes.emplace( p->getTypeGuid(), details::universal_type{ p->getTypeGuid(), p, p->getTypeName(), p->hasDeathmarchOn() } );
            }
        }

        //-------------------------------------------------------------------------

        void setUserData( void* pUserData, bool bOwnsUserData ) noexcept
        {
            m_pUserData     = pUserData;
            m_bOwnsUserData = bOwnsUserData;
        }

        //-------------------------------------------------------------------------

        template< typename T >
        T& getUserData( void ) noexcept
        {
            return *static_cast<T*>(m_pUserData);
        }

        //-------------------------------------------------------------------------
        template<auto RSC_TYPE_V>
        typename loader<RSC_TYPE_V>::data_type* RegisterResource(def_guid<RSC_TYPE_V>& Guid, loader<RSC_TYPE_V>::data_type* pRSC)
        {
            using data_type = typename loader<RSC_TYPE_V>::data_type;

            assert(Guid.m_Instance.isValid() && Guid.m_Instance.isPointer() == false);
            assert(pRSC);

            assert(m_ResourceInstance.find(Guid) == m_ResourceInstance.end());

            FullInstanceInfoAlloc(pRSC, Guid);

            return reinterpret_cast<data_type*>(Guid.m_Instance.m_Pointer = pRSC);
        }

        //-------------------------------------------------------------------------

        template< auto RSC_TYPE_V >
        typename loader<RSC_TYPE_V>::data_type* getResource( def_guid<RSC_TYPE_V>& R ) noexcept
        {
            using data_type = typename loader<RSC_TYPE_V>::data_type;

            // If we already have the xresource return now
            if (R.isValid() == false || R.m_Instance.isPointer()) return reinterpret_cast<data_type*>(R.m_Instance.m_Pointer);

            if( auto Entry = m_ResourceInstance.find(R); Entry != m_ResourceInstance.end() )
            {
                auto& E = *Entry->second;
                E.m_RefCount++;
                return reinterpret_cast<data_type*>(R.m_Instance.m_Pointer = E.m_pData);
            }

            // If the user return nulls it must mean that it failed to load so we could return a temporary xresource of the right type
            data_type* pRSC = loader<RSC_TYPE_V>::Load(*this, R);
            if (pRSC == nullptr) return nullptr;

            FullInstanceInfoAlloc(pRSC, R);

            return reinterpret_cast<data_type*>(R.m_Instance.m_Pointer = pRSC);
        }

        //-------------------------------------------------------------------------

        void* getResource( full_guid& URef ) noexcept
        {
            // If we already have the xresource return now
            if (URef.m_Instance.isPointer()) return URef.m_Instance.m_Pointer;

            if( auto Entry = m_ResourceInstance.find(URef); Entry != m_ResourceInstance.end() )
            {
                auto& E = *Entry->second;
                E.m_RefCount++;
                URef.m_Instance.m_Pointer = E.m_pData;
                return URef.m_Instance.m_Pointer;
            }

            auto UniversalType = m_RegisteredTypes.find(URef.m_Type);
            assert(UniversalType != m_RegisteredTypes.end()); // Type was not registered

            // If the user return nulls it must mean that it failed to load so we could return a temporary xresource of the right type
            void* pRSC = UniversalType->second.m_pRegistration->Load(*this, URef);
            if (pRSC == nullptr) return nullptr;

            FullInstanceInfoAlloc(pRSC, URef);

            return URef.m_Instance.m_Pointer = pRSC;
        }

        //-------------------------------------------------------------------------

        template< auto RSC_TYPE_V >
        void ReleaseRef(def_guid<RSC_TYPE_V>& Ref ) noexcept
        {
            if (Ref.m_Instance.isValid() == false || false == Ref.m_Instance.isPointer() ) return;

            auto S = m_ResourceInstanceRelease.find(reinterpret_cast<std::uint64_t>(Ref.m_Instance.m_Pointer));
            assert(S != m_ResourceInstanceRelease.end());

            auto& R = *S->second;
            --R.m_RefCount;
            auto OriginalGuid = R.m_Guid.m_Instance;
            assert(R.m_Guid.m_Type == Ref.m_Type);
            assert(R.m_pData == Ref.m_Instance.m_Pointer );

            //
            // If this is the last reference release the xresource
            //
            if( R.m_RefCount == 0 )
            {
                if (loader<RSC_TYPE_V>::use_death_march_v)
                {
                    auto& DestructionList = m_DeathMarchList[m_CurrentFrame % m_DeathMarchList.size()];
                    DestructionList.emplace_back(R.m_pData, R.m_Guid);
                }
                else
                {
                    loader<RSC_TYPE_V>::Destroy( *this, std::move(*static_cast<typename loader<RSC_TYPE_V>::data_type*>(R.m_pData)), R.m_Guid );
                }
                FullInstanceInfoRelease(R);
            }

            Ref.m_Instance = OriginalGuid;
        }

        //-------------------------------------------------------------------------

        void ReleaseRef( full_guid& URef ) noexcept
        {
            if (URef.m_Instance.isValid() == false || false == URef.m_Instance.isPointer()) return;

            auto S = m_ResourceInstanceRelease.find(reinterpret_cast<std::uint64_t>(URef.m_Instance.m_Pointer));
            assert(S != m_ResourceInstanceRelease.end());

            auto& R = *S->second;
            R.m_RefCount--;
            auto OriginalGuid = R.m_Guid.m_Instance;
            assert(URef.m_Type == R.m_Guid.m_Type);
            assert(R.m_pData == URef.m_Instance.m_Pointer);

            //
            // If this is the last reference release the xresource
            //
            if (R.m_RefCount == 0)
            {
                auto UniversalType = m_RegisteredTypes.find(URef.m_Type);
                assert(UniversalType != m_RegisteredTypes.end()); // Type was not registered

                if( UniversalType->second.m_bUseDeathMarch )
                {
                    auto& DestructionList = m_DeathMarchList[m_CurrentFrame % m_DeathMarchList.size()];
                    DestructionList.emplace_back(R.m_pData, R.m_Guid);
                }
                else
                {
                    UniversalType->second.m_pRegistration->Destroy(*this, R.m_pData, R.m_Guid);
                }
                FullInstanceInfoRelease(R);
            }

            URef.m_Instance = OriginalGuid;
        }

        //-------------------------------------------------------------------------

        template< auto RSC_TYPE_V >
        full_guid getFullGuid( const def_guid<RSC_TYPE_V>& R ) const noexcept
        {
            if (R.isValid() == false || false == R.m_Instance.isPointer()) return R;

            auto S = m_ResourceInstanceRelease.find(reinterpret_cast<std::uint64_t>(R.m_Instance.m_Pointer));
            assert(S != m_ResourceInstanceRelease.end());

            return S->second->m_Guid;
        }

        //-------------------------------------------------------------------------

        // When serializing resources of displaying them in the editor you may want to show the GUID rather than the pointer
        // When reference holds the pointer rather than the GUID we must find the actual GUID to return to the user
        full_guid getFullGuid( const full_guid& URef ) const noexcept
        {
            if (URef.isValid() == false || false == URef.m_Instance.isPointer()) return URef;

            auto S = m_ResourceInstanceRelease.find(reinterpret_cast<std::uint64_t>(URef.m_Instance.m_Pointer));
            assert(S != m_ResourceInstanceRelease.end());

            return S->second->m_Guid;
        }

        //-------------------------------------------------------------------------

        template<auto RSC_TYPE_V >
        void CloneRef( def_guid<RSC_TYPE_V>& Dest, const def_guid<RSC_TYPE_V>& Ref ) noexcept
        {
            if( Ref.isValid() && Ref.m_Instance.isPointer() )
            {
                if (Dest.isValid() && Dest.m_Instance.isPointer())
                {
                    if( Dest.m_Instance.m_Pointer == Ref.m_Instance.m_Pointer ) return;
                    ReleaseRef(Dest);
                }

                auto S = m_ResourceInstanceRelease.find(reinterpret_cast<std::uint64_t>(Ref.m_Instance.m_Pointer));
                assert(S != m_ResourceInstanceRelease.end());

                ++S->second->m_RefCount;
            }
            else
            {
                if (Dest.isValid() && Dest.m_Instance.isPointer()) ReleaseRef(Dest);
            }
            Dest.m_Instance = Ref.m_Instance;
        }

        //-------------------------------------------------------------------------

        void CloneRef(full_guid& Dest, const full_guid& URef ) noexcept
        {
            if(URef.m_Instance.isValid() && URef.m_Instance.isPointer() )
            {
                if (Dest.m_Instance.isValid() && Dest.m_Instance.isPointer())
                {
                    if (Dest.m_Instance.m_Pointer == URef.m_Instance.m_Pointer) return;
                    ReleaseRef(Dest);
                }

                auto S = m_ResourceInstanceRelease.find(reinterpret_cast<std::uint64_t>(URef.m_Instance.m_Pointer));
                assert(S != m_ResourceInstanceRelease.end());

                S->second->m_RefCount++;
            }
            else
            {
                if (Dest.m_Instance.isValid() && Dest.m_Instance.isPointer()) ReleaseRef(Dest);
            }

            Dest = URef;
        }

        //-------------------------------------------------------------------------

        int getResourceCount() const noexcept
        {
            assert( m_ResourceInstance.size() == m_ResourceInstanceRelease.size() );
            return static_cast<int>(m_ResourceInstance.size());
        }

        //-------------------------------------------------------------------------

        void setRootPath( std::wstring&& Path ) noexcept
        {
            m_RootPath = std::move(Path);
        }

        //-------------------------------------------------------------------------

        const std::wstring_view getRootPath( void ) const noexcept
        {
            return m_RootPath;
        }

        //-------------------------------------------------------------------------

        std::wstring getResourcePath( const xresource::full_guid& Guid, const std::wstring_view TypeName ) noexcept
        {
            // Make sure we get a valid guid
            assert(Guid.isValid() && Guid.m_Instance.isPointer() == false);

            return std::format(L"{}\\{}\\{:02X}\\{:02X}\\{:X}", m_RootPath, TypeName, (Guid.m_Instance.m_Value >> 0) & 0xff, (Guid.m_Instance.m_Value >> 8) & 0xff, Guid.m_Instance.m_Value);
        }

        //-------------------------------------------------------------------------

        std::wstring getResourcePath(const xresource::full_guid& Guid ) noexcept
        {
            assert(Guid.isValid() && Guid.m_Instance.isPointer()==false);

            auto UniversalType = m_RegisteredTypes.find(Guid.m_Type);

            // Type was not registered
            assert(UniversalType != m_RegisteredTypes.end());

            // get the final path
            return getResourcePath( Guid, UniversalType->second.m_TypeName );
        }

        //-------------------------------------------------------------------------

        void OnEndFrameDelegate()
        {
            m_CurrentFrame++;
            auto& DeathMarch = m_DeathMarchList[m_CurrentFrame % m_DeathMarchList.size()];
            for (auto& E : DeathMarch)
            {
                auto It = m_RegisteredTypes.find(E.m_FullGuid.m_Type);
                if (It != m_RegisteredTypes.end())
                {
                    It->second.m_pRegistration->Destroy(*this, E.m_pData, E.m_FullGuid);
                }
            }
            DeathMarch.clear();
        }

    protected:

        //-------------------------------------------------------------------------

        details::instance_info& AllocRscInfo( void ) noexcept
        {
            auto pTemp = m_pInfoBufferEmptyHead;
            assert(pTemp);

            details::instance_info* pNext = reinterpret_cast<details::instance_info*>(m_pInfoBufferEmptyHead->m_pData);
            m_pInfoBufferEmptyHead = pNext;

            return *pTemp;
        }

        //-------------------------------------------------------------------------

        void ReleaseRscInfo(details::instance_info& RscInfo) noexcept
        {
            // Add this xresource info to the empty chain
            RscInfo.m_pData = m_pInfoBufferEmptyHead;
            m_pInfoBufferEmptyHead = &RscInfo;
        }

        //-------------------------------------------------------------------------

        void FullInstanceInfoAlloc(void* pRsc, const full_guid& GUID) noexcept
        {
            auto& RscInfo = AllocRscInfo();

            RscInfo.m_pData     = pRsc;
            RscInfo.m_Guid      = GUID;
            RscInfo.m_RefCount  = 1;

            m_ResourceInstance.emplace(GUID, &RscInfo );
            m_ResourceInstanceRelease.emplace( reinterpret_cast<std::uint64_t>(pRsc), &RscInfo );
        }

        //-------------------------------------------------------------------------

        void FullInstanceInfoRelease( details::instance_info& RscInfo ) noexcept
        {
            // Release references in the hashs maps
            m_ResourceInstance.erase( RscInfo.m_Guid );
            m_ResourceInstanceRelease.erase( reinterpret_cast<std::uint64_t>(RscInfo.m_pData) );

            // Add this xresource info to the empty chain
            ReleaseRscInfo(RscInfo);
        }

        struct death_march_entry
        {
            void*                   m_pData;
            xresource::full_guid    m_FullGuid;
        };

        //-------------------------------------------------------------------------
        //-------------------------------------------------------------------------

        std::unordered_map<type_guid, details::universal_type>      m_RegisteredTypes           = {};
        std::unordered_map<full_guid, details::instance_info*>      m_ResourceInstance          = {};
        std::unordered_map<std::uint64_t, details::instance_info*>  m_ResourceInstanceRelease   = {};
        details::instance_info*                                     m_pInfoBufferEmptyHead      = { nullptr };
        std::unique_ptr<details::instance_info[]>                   m_InfoBuffer                = {};
        std::size_t                                                 m_MaxResources              = {};
        std::wstring                                                m_RootPath                  = {};
        std::array<std::vector<death_march_entry>,2>                m_DeathMarchList            = {};
        int                                                         m_CurrentFrame              = 0;
        void*                                                       m_pUserData                 = {};
        bool                                                        m_bOwnsUserData             = {false};
    };
}
#endif
