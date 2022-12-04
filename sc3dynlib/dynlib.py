import shlex
import subprocess
from sc3.synth.ugen import MultiOutUGen
from sc3.synth.ugens import install_ugens_module
import tempfile


def build_snippet(name, code_string):
    with tempfile.NamedTemporaryFile('w', suffix='.cpp') as tmp:
        tmp.write(code_string)
        tmp.flush()
        # TODO building alternatives
        command1 = f'g++ -fpic -c {tmp.name} -o {name}.o -Ofast'
        command2 = f'gcc -fpic -shared {name}.o -lm -o lib{name}.so -Ofast'
        # TODO return errors
        out1 = subprocess.call(shlex.split(command1))
        print(out1)
        subprocess.call(shlex.split(command2))

# TODO mono n_outputs=1 not working, only >=2 
class Dynlib(MultiOutUGen):
    @classmethod
    def ar(self, label=None, *inputs, n_outputs=2):
        self.n_outputs = n_outputs
        return self._multi_new('audio', label, *inputs)

    @classmethod
    def _new1(self, rate, label, *args):
        label = label or 'default'
        label = [int(x) for x in bytes(label, 'utf-8')]
        inputs = [len(label), *label, *args]
        print(inputs)
        obj = self._create_ugen_object(rate)
        obj._add_to_synth()
        return obj._init_ugen(*inputs)

    def _init_ugen(self, *inputs):
        self._inputs = inputs
        return self._init_outputs(self.n_outputs, self.rate)


install_ugens_module(__name__)
