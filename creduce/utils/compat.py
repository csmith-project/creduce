import importlib.util
import signal
import subprocess
import sys
import warnings as _warnings

if sys.version_info >= (3, 5):
    subprocess_run = subprocess.run
    importlib_module_from_spec = importlib.util.module_from_spec
else:
    class CompletedProcess(object):
        """A process that has finished running.

        This is returned by run().

        Attributes:
          args: The list or str args passed to run().
          returncode: The exit code of the process, negative for signals.
          stdout: The standard output (None if not captured).
          stderr: The standard error (None if not captured).
        """
        def __init__(self, args, returncode, stdout=None, stderr=None):
            self.args = args
            self.returncode = returncode
            self.stdout = stdout
            self.stderr = stderr

        def __repr__(self):
            args = ['args={!r}'.format(self.args),
                    'returncode={!r}'.format(self.returncode)]
            if self.stdout is not None:
                args.append('stdout={!r}'.format(self.stdout))
            if self.stderr is not None:
                args.append('stderr={!r}'.format(self.stderr))
            return "{}({})".format(type(self).__name__, ', '.join(args))

        def check_returncode(self):
            """Raise CalledProcessError if the exit code is non-zero."""
            if self.returncode:
                raise CalledProcessError(self.returncode, self.args, self.stdout,
                                         self.stderr)

    class CalledProcessError(subprocess.CalledProcessError):
        """Raised when a check_call() or check_output() process returns non-zero.

        The exit status will be stored in the returncode attribute, negative
        if it represents a signal number.

        check_output() will also store the output in the output attribute.
        """
        def __init__(self, returncode, cmd, output=None, stderr=None):
            self.returncode = returncode
            self.cmd = cmd
            self.output = output
            self.stderr = stderr

        def __str__(self):
            if self.returncode and self.returncode < 0:
                try:
                    return "Command '%s' died with %r." % (
                            self.cmd, signal.Signals(-self.returncode))
                except ValueError:
                    return "Command '%s' died with unknown signal %d." % (
                            self.cmd, -self.returncode)
            else:
                return "Command '%s' returned non-zero exit status %d." % (
                        self.cmd, self.returncode)

        @property
        def stdout(self):
            """Alias for output attribute, to match stderr"""
            return self.output

        @stdout.setter
        def stdout(self, value):
            # There's no obvious reason to set this, but allow it anyway so
            # .stdout is a transparent alias for .output
            self.output = value

    def subprocess_run(*popenargs, input=None, timeout=None, check=False, **kwargs):
        if sys.version_info < (3, 5):
            if input is not None:
                if 'stdin' in kwargs:
                    raise ValueError('stdin and input arguments may not both be used.')
                kwargs['stdin'] = PIPE

            with subprocess.Popen(*popenargs, **kwargs) as process:
                try:
                    stdout, stderr = process.communicate(input, timeout=timeout)
                except subprocess.TimeoutExpired:
                    process.kill()
                    stdout, stderr = process.communicate()
                    raise subprocess.TimeoutExpired(process.args, timeout, output=stdout,
                                         stderr=stderr)
                except:
                    process.kill()
                    process.wait()
                    raise
                retcode = process.poll()
                if check and retcode:
                    raise CalledProcessError(retcode, process.args,
                                             output=stdout, stderr=stderr)
            return CompletedProcess(process.args, retcode, stdout, stderr)

    def _new_module(name):
        return type(sys)(name)

    def _init_module_attrs(spec, module, *, override=False):
        # The passed-in module may be not support attribute assignment,
        # in which case we simply don't set the attributes.
        # __name__
        if (override or getattr(module, '__name__', None) is None):
            try:
                module.__name__ = spec.name
            except AttributeError:
                pass
        # __loader__
        if override or getattr(module, '__loader__', None) is None:
            loader = spec.loader
            if loader is None:
                # A backward compatibility hack.
                if spec.submodule_search_locations is not None:
                    if _bootstrap_external is None:
                        raise NotImplementedError
                    _NamespaceLoader = _bootstrap_external._NamespaceLoader

                    loader = _NamespaceLoader.__new__(_NamespaceLoader)
                    loader._path = spec.submodule_search_locations
            try:
                module.__loader__ = loader
            except AttributeError:
                pass
        # __package__
        if override or getattr(module, '__package__', None) is None:
            try:
                module.__package__ = spec.parent
            except AttributeError:
                pass
        # __spec__
        try:
            module.__spec__ = spec
        except AttributeError:
            pass
        # __path__
        if override or getattr(module, '__path__', None) is None:
            if spec.submodule_search_locations is not None:
                try:
                    module.__path__ = spec.submodule_search_locations
                except AttributeError:
                    pass
        # __file__/__cached__
        if spec.has_location:
            if override or getattr(module, '__file__', None) is None:
                try:
                    module.__file__ = spec.origin
                except AttributeError:
                    pass

            if override or getattr(module, '__cached__', None) is None:
                if spec.cached is not None:
                    try:
                        module.__cached__ = spec.cached
                    except AttributeError:
                        pass
        return module

    def importlib_module_from_spec(spec):
        """Create a module based on the provided spec."""
        # Typically loaders will not implement create_module().
        module = None
        if hasattr(spec.loader, 'create_module'):
            # If create_module() returns `None` then it means default
            # module creation should be used.
            module = spec.loader.create_module(spec)
        elif hasattr(spec.loader, 'exec_module'):
            _warnings.warn('starting in Python 3.6, loaders defining exec_module() '
                           'must also define create_module()',
                           DeprecationWarning, stacklevel=2)
        if module is None:
            module = _new_module(spec.name)
        _init_module_attrs(spec, module)
        return module
