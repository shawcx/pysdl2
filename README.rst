
SDL2
====

Python3 bindings for libSDL2 (plus SDL2_image and, optionally, SDL2_ttf),
exposed as a single C extension
module named ``SDL2``. Wrapped functions drop the ``SDL_`` prefix:
``SDL_GetPlatform`` is ``SDL2.GetPlatform``, ``SDL_RenderPresent`` is
``Renderer.Present``.

Dependencies
------------

Debian / Ubuntu
	* apt install libsdl2-dev libsdl2-image-dev
	* optional text rendering: apt install libsdl2-ttf-dev

macOS
	* brew install sdl2 sdl2_image
	* optional text rendering: brew install sdl2_ttf

SDL2_ttf is picked up automatically when installed and provides ``SDL2.Font``;
set ``PYSDL_TTF=0`` when building to leave it out.

Building
--------

	* ``pip install .`` -- build and install
	* ``python3 setup.py build`` -- build in place under ``build/``
	* ``make`` -- build a Debian package into ``deb_dist/`` (needs ``stdeb``)

Example
-------

.. code-block:: python

    import SDL2

    SDL2.Init()
    window = SDL2.Window('hello', (640, 480))
    renderer = window.CreateRenderer()

    running = True
    while running:
        while (event := SDL2.PollEvent()) is not None:
            if event[0] == SDL2.QUIT:
                running = False
        renderer.SetRenderDrawColor(32, 32, 64)
        renderer.Clear()
        renderer.Present()
        SDL2.Delay(16)

    SDL2.Quit()

More programs live in ``example/``.

Tests
-----

``pip install pytest`` then ``python3 -m pytest``. The suite runs headless with
SDL's ``dummy`` video and audio drivers.
