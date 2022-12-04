import setuptools


with open('README.md', 'r') as f:
    long_description = f.read()

setuptools.setup(
    name='sc3dynlib',
    version=0.1,
    author='Pablo Riera',
    author_email='pabloriera@gmail.com',
    license='GPLv3',
    platforms='Any',
    description='On the fly C plugins for SC3 SuperCollider library for Python',
    long_description=long_description,
    long_description_content_type='text/markdown',
    url='https://github.com/pabloriera/sc3dyblib',
    packages=[
        'sc3dynlib'
    ],
    install_requires=['sc3'],
    python_requires='>=3.7',
    classifiers=[
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
        'License :: OSI Approved :: GNU General Public License v3 (GPLv3)',
        'Operating System :: OS Independent',
        'Development Status :: 3 - Alpha'
    ],
    keywords='JIT on the fly C SuperCollider sound synthesis music-composition',
)