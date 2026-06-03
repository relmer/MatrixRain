#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\AdapterSelection.h"
#include "..\..\MatrixRainCore\InMemoryAdapterProvider.h"




namespace MatrixRainTests
{


    static AdapterInfo MakeAdapter (const std::wstring & description, LONG luidLo, bool isDefault, unsigned int vramMb = 1024)
    {
        AdapterInfo a;
        a.m_description     = description;
        a.m_luid            = LUID { static_cast<DWORD> (luidLo), 0 };
        a.m_dedicatedVramMb = vramMb;
        a.m_isSoftware      = false;
        a.m_isDefault       = isDefault;
        return a;
    }




    static bool LuidEquals (LUID a, LUID b)
    {
        return a.LowPart == b.LowPart && a.HighPart == b.HighPart;
    }




    TEST_CLASS (AdapterSelectionTests)
    {
        public:

            //
            //  ResolveAdapter
            //

            TEST_METHOD (ResolveAdapter_EmptySaved_ReturnsNullopt)
            {
                std::vector<AdapterInfo> adapters {
                    MakeAdapter (L"Intel UHD Graphics 620", 100, true),
                    MakeAdapter (L"NVIDIA GeForce GTX 1650", 200, false)
                };

                auto result = ResolveAdapter (adapters, L"");

                Assert::IsFalse (result.has_value());
            }




            TEST_METHOD (ResolveAdapter_EmptyAdapterList_ReturnsNullopt)
            {
                std::vector<AdapterInfo> adapters;

                auto result = ResolveAdapter (adapters, L"NVIDIA GeForce GTX 1650");

                Assert::IsFalse (result.has_value());
            }




            TEST_METHOD (ResolveAdapter_NonMatchingSaved_ReturnsNullopt)
            {
                std::vector<AdapterInfo> adapters {
                    MakeAdapter (L"Intel UHD Graphics 620", 100, true)
                };

                auto result = ResolveAdapter (adapters, L"Acme GPU 9000");

                Assert::IsFalse (result.has_value());
            }




            TEST_METHOD (ResolveAdapter_MatchingSaved_ReturnsLuid)
            {
                std::vector<AdapterInfo> adapters {
                    MakeAdapter (L"Intel UHD Graphics 620",  100, true),
                    MakeAdapter (L"NVIDIA GeForce GTX 1650", 200, false)
                };

                auto result = ResolveAdapter (adapters, L"NVIDIA GeForce GTX 1650");

                Assert::IsTrue   (result.has_value());
                Assert::IsTrue   (LuidEquals (*result, LUID { 200, 0 }));
            }




            TEST_METHOD (ResolveAdapter_DuplicateDescriptions_ReturnsFirstMatch)
            {
                std::vector<AdapterInfo> adapters {
                    MakeAdapter (L"Acme GPU", 100, true),
                    MakeAdapter (L"Acme GPU", 200, false)
                };

                auto result = ResolveAdapter (adapters, L"Acme GPU");

                Assert::IsTrue   (result.has_value());
                Assert::IsTrue   (LuidEquals (*result, LUID { 100, 0 }));
            }




            //
            //  FormatAdapterLabel
            //

            TEST_METHOD (FormatAdapterLabel_NonDefault_ReturnsDescriptionUnchanged)
            {
                AdapterInfo a = MakeAdapter (L"NVIDIA GeForce GTX 1650", 200, false);

                Assert::AreEqual (std::wstring (L"NVIDIA GeForce GTX 1650"), FormatAdapterLabel (a));
            }




            TEST_METHOD (FormatAdapterLabel_Default_AppendsSuffix)
            {
                AdapterInfo a = MakeAdapter (L"Intel UHD Graphics 620", 100, true);

                Assert::AreEqual (std::wstring (L"Intel UHD Graphics 620 (default)"), FormatAdapterLabel (a));
            }




            TEST_METHOD (FormatAdapterLabel_EmptyDescriptionDefault_ReturnsSuffixOnly)
            {
                AdapterInfo a = MakeAdapter (L"", 0, true);

                Assert::AreEqual (std::wstring (L" (default)"), FormatAdapterLabel (a));
            }




            //
            //  InMemoryAdapterProvider
            //

            TEST_METHOD (InMemoryAdapterProvider_ReturnsCopyOfInput)
            {
                std::vector<AdapterInfo> input {
                    MakeAdapter (L"Intel UHD Graphics 620",  100, true),
                    MakeAdapter (L"NVIDIA GeForce GTX 1650", 200, false)
                };

                InMemoryAdapterProvider provider (input);
                auto                    enumerated = provider.EnumerateAdapters();

                Assert::AreEqual (size_t (2), enumerated.size());
                Assert::AreEqual (std::wstring (L"Intel UHD Graphics 620"),  enumerated[0].m_description);
                Assert::AreEqual (std::wstring (L"NVIDIA GeForce GTX 1650"), enumerated[1].m_description);
                Assert::IsTrue   (enumerated[0].m_isDefault);
                Assert::IsFalse  (enumerated[1].m_isDefault);
            }
    };


}
