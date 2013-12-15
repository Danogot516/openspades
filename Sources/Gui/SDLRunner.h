/*
 Copyright (c) 2013 yvt
 
 This file is part of OpenSpades.
 
 OpenSpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 OpenSpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with OpenSpades.  If not, see <http://www.gnu.org/licenses/>.
 
 */

#pragma once

#include <Core/IRunnable.h>
#include <Imports/SDL.h>
#include <string>
#include <Core/ServerAddress.h>

namespace spades {
	namespace client{
		class IRenderer;
		class IAudioDevice;
	}
	namespace gui {
		class View;
		class SDLRunner: public IRunnable {
			bool mActive;
		protected:
			std::string TranslateKey(const SDL_Keysym&);
			std::string TranslateButton(Uint8 b);
			virtual int GetModState();
			void ProcessEvent(SDL_Event& event,
							  View *);
			virtual void RunClientLoop(client::IRenderer *renderer, client::IAudioDevice *dev);
			virtual View *CreateView(client::IRenderer *renderer, client::IAudioDevice *dev) = 0;
			virtual client::IAudioDevice *CreateAudioDevice();
		public:
			SDLRunner();
			virtual ~SDLRunner();
			virtual void Run();
		};
	}
}
