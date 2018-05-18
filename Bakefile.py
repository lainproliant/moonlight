import bakery.recipes.file as File
import bakery.recipes.cxx as CXX

CXX.CXX = 'clang++'
CXX.CFLAGS = [
    '-g',
    '--std=c++17',
    '-I./include'
]

@recipe('executable', check='src')
async def compile_and_link(src, executable):
    object_file = await CXX.compile(src, executable + '.o')
    executable = await CXX.link(object_file, executable)
    File.remove(object_file)
    return executable

@build
class Moonlight:
    @provide
    def temp(self):
        return File.temp_directory('test/temp')

    @provide
    def test_sources(self):
        return File.glob('test/*.cpp')

    @provide
    def tests(self, test_sources):
        return [compile_and_link(src, File.drop_ext(src)) for src in test_sources]

    @default
    @noclean
    async def run_tests(self, temp, tests):
        for test in tests:
            await shell(File.abspath(test), cwd='test')
