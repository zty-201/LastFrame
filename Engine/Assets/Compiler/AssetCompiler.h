#ifndef ASSET_COMPILER_H
#define ASSET_COMPILER_H


namespace EngineFramework {

    class IAssetCompiler
    {
    public:
        virtual ~IAssetCompiler() = default;

        virtual bool NeedsCompilation(
            const std::filesystem::path& source,
            const std::filesystem::path& output) const = 0;

        virtual bool Compile(
            const std::filesystem::path& source,
            const std::filesystem::path& output) const = 0;

        virtual std::string GetSourceExt() const = 0;
        virtual std::string GetOutputExt() const = 0;
    };

} // namespace EngineFramework

#endif