/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "sherlock/tattoo/tattoo_people.h"
#include "sherlock/tattoo/tattoo_scene.h"
#include "sherlock/tattoo/tattoo_talk.h"
#include "sherlock/sherlock.h"

namespace Sherlock {

namespace Tattoo {

#define FACING_PLAYER 16

static const int WALK_SPEED_X[99] = {
	90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 98, 90, 90, 90, 90, 90, 91, 90, 90,
	90, 90,100, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90,100, 90,
	90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90,103, 90, 90, 90, 90, 90, 90, 90,
	90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90,
	90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90
};

static const int WALK_SPEED_Y[99] = {
	28, 28, 28, 28, 28, 28, 28, 28, 28, 32, 32, 32, 28, 28, 28, 28, 28, 26, 28, 28,
	28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
	32, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 31, 28, 28, 28, 28, 28, 28, 28,
	28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
	28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28
};

static const int WALK_SPEED_DIAG_X[99] = {
	50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
	50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
	50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
	50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 90, 50, 50, 50, 50, 50, 50, 50, 50, 50,
	50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50
};

/*----------------------------------------------------------------*/

TattooPerson::TattooPerson() : Person() {
	Common::fill(&_npcPath[0], &_npcPath[MAX_NPC_PATH], 0);
	_tempX = _tempScaleVal = 0;
	_npcIndex = 0;
	_npcStack = 0;
	_npcMoved = false;
	_npcFacing = -1;
	_resetNPCPath = true;
	_savedNpcSequence = 0;
	_savedNpcFrame = 0;
	_updateNPCPath = false;
	_npcPause = false;
}

void TattooPerson::adjustSprite() {
	People &people = *_vm->_people;
	TattooScene &scene = *(TattooScene *)_vm->_scene;

	if (_type == INVALID)
		return;

	if (_type == CHARACTER && _status) {
		// Sprite waiting to move, so restart walk
		_walkCount = _status;
		_status = 0;

		people._walkDest = _walkTo.front();
		setWalking();
	} else if (_type == CHARACTER && _walkCount) {
		if (_walkCount > 10) {
			people._walkDest = _walkTo.front();
			setWalking();
		}

		_position += _delta;
		if (_walkCount)
			--_walkCount;

		if (!_walkCount) {
			// If there are remaining points to walk, move to the next one
			if (!_walkTo.empty()) {
				people._walkDest = _walkTo.pop();
				setWalking();
			} else {
				gotoStand();
			}
		}
	}

	if (_type != CHARACTER) {
		if (_position.y > SHERLOCK_SCREEN_HEIGHT)
			_position.y = SHERLOCK_SCREEN_HEIGHT;

		if (_position.y < UPPER_LIMIT)
			_position.y = UPPER_LIMIT;

		if (_position.x < LEFT_LIMIT)
			_position.x = LEFT_LIMIT;

		if (_position.x > RIGHT_LIMIT)
			_position.x = RIGHT_LIMIT;
	}

	int frameNum = _frameNumber;
	if (frameNum == -1)
		frameNum = 0;
	int idx = _walkSequences[_sequenceNumber][frameNum];
	if (idx > _maxFrames)
		idx = 1;

	// Set the image frame
	if (_altSeq)
		_imageFrame = &(*_altImages)[idx - 1];
	else
		_imageFrame = &(*_images)[idx - 1];

	// See if the player has come to a stop after clicking on an Arrow zone to leave the scene.
	// If so, this will set up the exit information for the scene transition
	if (!_walkCount && scene._exitZone != -1 && scene._walkedInScene && scene._goToScene != -1 &&
			!_description.compareToIgnoreCase(people[HOLMES]._description)) { 
		people._hSavedPos = scene._exits[scene._exitZone]._newPosition;
		people._hSavedFacing = scene._exits[scene._exitZone]._newFacing;

		if (people._hSavedFacing > 100 && people._hSavedPos.x < 1)
			people._hSavedPos.x = 100;
	}
}

void TattooPerson::gotoStand() {
	TattooPeople &people = *(TattooPeople *)_vm->_people;

	// If the misc field is set, then we're running a special talk sequence, so don't interrupt it.
	if (_misc)
		return;

	_walkTo.clear();
	_walkCount = 0;
	int oldFacing = _sequenceNumber;

	// If the person was talking or listening, just return it to the standing sequence 
	// in the direction they were pointing
	if (_sequenceNumber >= TALK_UPRIGHT && _sequenceNumber <= LISTEN_UPLEFT) {
		switch (_sequenceNumber) {
		case TALK_UPRIGHT:
		case LISTEN_UPRIGHT:
			_sequenceNumber = STOP_UPRIGHT;
			break;
		case TALK_RIGHT:
		case LISTEN_RIGHT:
			_sequenceNumber = STOP_RIGHT;
			break;
		case TALK_DOWNRIGHT:
		case LISTEN_DOWNRIGHT:
			_sequenceNumber = STOP_DOWNRIGHT;
			break;
		case TALK_DOWNLEFT:
		case LISTEN_DOWNLEFT:
			_sequenceNumber = STOP_DOWNLEFT;
			break;
		case TALK_LEFT:
		case LISTEN_LEFT:
			_sequenceNumber = STOP_LEFT;
			break;
		case TALK_UPLEFT:
		case LISTEN_UPLEFT:
			_sequenceNumber = STOP_UPLEFT;
			break;
		default:
			break;
		}

		if (_seqTo) {
			// Reset to previous value
			_walkSequences[oldFacing]._sequences[_frameNumber] = _seqTo;
			_seqTo = 0;
		}

		// Set the Frame number to the last frame so we don't move
		_frameNumber = 0;

		checkWalkGraphics();

		_oldWalkSequence = -1;
		people._allowWalkAbort = true;
		return;
	}

	// If the sprite that is stopping is an NPC and he is supposed to face a certain direction 
	// when he stops, set that direction here
	int npc = -1;
	for (int idx = 1; idx < MAX_CHARACTERS; ++idx) {
		if (_imageFrame == people[idx]._imageFrame)
			npc = idx;
	}

	if (npc != -1 && people[npc]._npcFacing != -1) {
		if (people[npc]._npcFacing == FACING_PLAYER) {
			// See where Holmes is with respect to the NPC (x coords)
			if (people[HOLMES]._position.x < people[npc]._position.x)
				people[npc]._npcFacing = STOP_LEFT;
			else
				people[npc]._npcFacing = STOP_RIGHT;

			// See where Holmes is with respect to the NPC (y coords)
			if (people[HOLMES]._position.y < people[npc]._position.y - (10 * FIXED_INT_MULTIPLIER)) {
				// Holmes is above the NPC so reset the facing to the diagonal ups
				if (people[npc]._npcFacing == STOP_RIGHT)
					people[npc]._npcFacing = STOP_UPRIGHT;
				else
					people[npc]._npcFacing = STOP_UPLEFT;
			} else {
				if (people[HOLMES]._position.y > people[npc]._position.y + (10 * FIXED_INT_MULTIPLIER)) {
					// Holmes is below the NPC so reset the facing to the diagonal downs
					if (people[npc]._npcFacing == STOP_RIGHT)
						people[npc]._npcFacing = STOP_DOWNRIGHT;
					else
						people[npc]._npcFacing = STOP_DOWNLEFT;
				}
			}
		}

		_sequenceNumber = people[npc]._npcFacing;
	} else {
		switch (_sequenceNumber) {
		case WALK_UP: _sequenceNumber = STOP_UP;			break;
		case WALK_UPRIGHT: _sequenceNumber = STOP_UPRIGHT;	break;
		case WALK_RIGHT: _sequenceNumber = STOP_RIGHT;		break;
		case WALK_DOWNRIGHT: _sequenceNumber = STOP_DOWNRIGHT; break;
		case WALK_DOWN: _sequenceNumber = STOP_DOWN;		break;
		case WALK_DOWNLEFT: _sequenceNumber = STOP_DOWNLEFT;break;
		case WALK_LEFT: _sequenceNumber = STOP_LEFT;		break;
		case WALK_UPLEFT: _sequenceNumber = STOP_UPLEFT;	break;
		}
	}

	// Only restart the frame number at 0 if the new sequence is different from the last sequence 
	// so we don't let Holmes repeat standing.
	if (_oldWalkSequence != -1) {
		if (_seqTo) {
			// Reset to previous value
			_walkSequences[oldFacing]._sequences[_frameNumber] = _seqTo;
			_seqTo = 0;
		}

		_frameNumber = 0;
	}

	checkWalkGraphics();

	_oldWalkSequence = -1;
	people._allowWalkAbort = true;
}

void TattooPerson::setWalking() {
	People &people = *_vm->_people;
	TattooScene &scene = *(TattooScene *)_vm->_scene;
	int oldDirection, oldFrame;
	Common::Point delta;

	// Flag that player has now walked in the scene
	scene._walkedInScene = true;

	// Stop any previous walking, since a new dest is being set
	_walkCount = 0;
	oldDirection = _sequenceNumber;
	oldFrame = _frameNumber;

	// Set speed to use horizontal and vertical movement
	int scaleVal = scene.getScaleVal(_position);
	Common::Point speed(MAX(WALK_SPEED_X[scene._currentScene - 1] * SCALE_THRESHOLD / scaleVal, 2),
		MAX(WALK_SPEED_Y[scene._currentScene - 1] * SCALE_THRESHOLD / scaleVal, 2));
	Common::Point diagSpeed(MAX(WALK_SPEED_DIAG_X[scene._currentScene - 1] * SCALE_THRESHOLD / scaleVal, 2),
		MAX((WALK_SPEED_Y[scene._currentScene - 1] - 2) * SCALE_THRESHOLD / scaleVal, 2));

	// If the player is already close to the given destination that no walking is needed, 
	// move to the next  straight line segment in the overall walking route, if there is one
	for (;;) {
		if (_centerWalk || !_walkTo.empty()) {
			// Since we want the player to be centered on the ultimate destination, and the player
			// is drawn from the left side, move the cursor half the width of the player to center it
			delta = Common::Point(_position.x / FIXED_INT_MULTIPLIER - people._walkDest.x,
				_position.y / FIXED_INT_MULTIPLIER - people._walkDest.y);

			int dir;
			if (ABS(delta.x) > ABS(delta.y))
				dir = (delta.x < 0) ? WALK_LEFT : WALK_RIGHT;
			else
				dir = (delta.y < 0) ? WALK_UP : WALK_DOWN;

			int scaleVal = scene.getScaleVal(Point32(people._walkDest.x * FIXED_INT_MULTIPLIER,
				people._walkDest.y * FIXED_INT_MULTIPLIER));
			people._walkDest.x -= _stopFrames[dir]->sDrawXSize(scaleVal) / 2;
		}

		delta = Common::Point(
			ABS(_position.x / FIXED_INT_MULTIPLIER - people._walkDest.x),
			ABS(_position.y / FIXED_INT_MULTIPLIER - people._walkDest.y)
			);

		// If we're ready to move a sufficient distance, that's it. Otherwise,
		// move onto the next portion of the walk path, if there is one
		if ((delta.x > 3 || delta.y > 0) || _walkTo.empty())
			break;

		// Pop next walk segment off the walk route stack
		people._walkDest = _walkTo.pop();
	}

	// If a sufficient move is being done, then start the move
	if (delta.x > 3 || delta.y) {
		// See whether the major movement is horizontal or vertical
		if (delta.x >= delta.y) {
			// Set the initial frame sequence for the left and right, as well
			// as setting the delta x depending on direction
			if (people._walkDest.x < (_position.x / FIXED_INT_MULTIPLIER)) {
				_sequenceNumber = WALK_LEFT;
				_delta.x = speed.x * -(FIXED_INT_MULTIPLIER / 10);
			} else {
				_sequenceNumber = WALK_RIGHT;
				_delta.x = speed.x * (FIXED_INT_MULTIPLIER / 10);
			}

			// See if the x delta is too small to be divided by the speed, since
			// this would cause a divide by zero error
			if ((delta.x * 10) >= speed.x) {
				// Det the delta y
				_delta.y = (delta.y * FIXED_INT_MULTIPLIER) / ((delta.x * 10) / speed.x);
				if (people._walkDest.y < (_position.y / FIXED_INT_MULTIPLIER))
					_delta.y = -_delta.y;

				// Set how many times we should add the delta to the player's position
				_walkCount = (delta.x * 10) / speed.x;
			} else {
				// The delta x was less than the speed (ie. we're really close to
				// the destination). So set delta to 0 so the player won't move
				_delta = Point32(0, 0);
				_position = Point32(people._walkDest.x * FIXED_INT_MULTIPLIER, people._walkDest.y * FIXED_INT_MULTIPLIER);

				_walkCount = 1;
			}

			// See if the sequence needs to be changed for diagonal walking
			if (_delta.y > 1500) {
				if (_sequenceNumber == WALK_LEFT || _sequenceNumber == WALK_RIGHT) {
					_delta.x = _delta.x / speed.x * diagSpeed.x;
					_delta.y = (delta.y * FIXED_INT_MULTIPLIER) / (delta.x * 10 / diagSpeed.x);
				}

				switch (_sequenceNumber) {
				case WALK_LEFT:
					_sequenceNumber = WALK_DOWNLEFT;
					break;
				case WALK_RIGHT:
					_sequenceNumber = WALK_DOWNRIGHT;
					break;
				}
			} else if (_delta.y < -1500) {
				if (_sequenceNumber == WALK_LEFT || _sequenceNumber == WALK_RIGHT) {
					_delta.x = _delta.x / speed.x * diagSpeed.x;
					_delta.y = -1 * (delta.y * FIXED_INT_MULTIPLIER) / (delta.x * 10 / diagSpeed.x);
					_walkCount = (delta.x * 10) / diagSpeed.x;
				}

				switch (_sequenceNumber) {
				case WALK_LEFT:
					_sequenceNumber = WALK_UPLEFT;
					break;
				case WALK_RIGHT:
					_sequenceNumber = WALK_UPRIGHT;
					break;
				}
			}
		} else {
			// Major movement is vertical, so set the sequence for up and down,
			// and set the delta Y depending on the direction
			if (people._walkDest.y < (_position.y / FIXED_INT_MULTIPLIER)) {
				_sequenceNumber = WALK_UP;
				_delta.y = speed.y * -FIXED_INT_MULTIPLIER;
			} else {
				_sequenceNumber = WALK_DOWN;
				_delta.y = speed.y * FIXED_INT_MULTIPLIER;
			}

			// Set the delta x
			_delta.x = (delta.x * FIXED_INT_MULTIPLIER) / (delta.y / speed.y);
			if (people._walkDest.x < (_position.x / FIXED_INT_MULTIPLIER))
				_delta.x = -_delta.x;

			_walkCount = delta.y / speed.y;
		}
	}

	// See if the new walk sequence is the same as the old. If it's a new one,
	// we need to reset the frame number to zero so it's animation starts at
	// it's beginning. Otherwise, if it's the same sequence, we can leave it
	// as is, so it keeps the animation going at wherever it was up to
	if (_sequenceNumber != _oldWalkSequence) {
		if (_seqTo) {
			// Reset to previous value
			_walkSequences[oldDirection]._sequences[_frameNumber] = _seqTo;
			_seqTo = 0;
		}
		_frameNumber = 0;
	}

	checkWalkGraphics();
	_oldWalkSequence = _sequenceNumber;

	if (!_walkCount && _walkTo.empty())
		gotoStand();

	// If the sequence is the same as when we started, then Holmes was standing still and we're trying 
	// to re-stand him, so reset Holmes' rame to the old frame number from before it was reset to 0
	if (_sequenceNumber == oldDirection)
		_frameNumber = oldFrame;
}

void TattooPerson::clearNPC() {
	Common::fill(&_npcPath[0], &_npcPath[MAX_NPC_PATH], 0);
	_npcIndex = _npcStack = 0;
	_npcName = "";
}

void TattooPerson::updateNPC() {
	warning("TODO: updateNPC");
}

void TattooPerson::pushNPCPath() {
	warning("TODO: pushNPCPath");
}

Common::Point TattooPerson::getSourcePoint() const {
	TattooScene &scene = *(TattooScene *)_vm->_scene;
	int scaleVal = scene.getScaleVal(_position);

	return Common::Point(_position.x / FIXED_INT_MULTIPLIER + _imageFrame->sDrawXSize(scaleVal) / 2,
		_position.y / FIXED_INT_MULTIPLIER);
}

/*----------------------------------------------------------------*/

TattooPeople::TattooPeople(SherlockEngine *vm) : People(vm) {
	for (int idx = 0; idx < 6; ++idx)
		_data.push_back(new TattooPerson());
}

void TattooPeople::setListenSequence(int speaker, int sequenceNum) {
	Scene &scene = *_vm->_scene;

	// If no speaker is specified, then nothing needs to be done
	if (speaker == -1)
		return;

	int objNum = findSpeaker(speaker);
	if (objNum < 256 && objNum != -1) {
		// See if the Object has to wait for an Abort Talk Code
		Object &obj = scene._bgShapes[objNum];
		if (obj.hasAborts())
			obj._gotoSeq = sequenceNum;
		else
			obj.setObjTalkSequence(sequenceNum);
	} else if (objNum != -1) {
		objNum -= 256;
		Person &person = *_data[objNum];

		int newDir = person._sequenceNumber;
		switch (person._sequenceNumber) {
		case WALK_UP:
		case STOP_UP:
		case WALK_UPRIGHT:
		case STOP_UPRIGHT:
		case TALK_UPRIGHT:
		case LISTEN_UPRIGHT:
			newDir = LISTEN_UPRIGHT;
			break;
		case WALK_RIGHT:
		case STOP_RIGHT:
		case TALK_RIGHT:
		case LISTEN_RIGHT:
			newDir = LISTEN_RIGHT;
			break;
		case WALK_DOWNRIGHT:
		case STOP_DOWNRIGHT:
		case TALK_DOWNRIGHT:
		case LISTEN_DOWNRIGHT:
			newDir = LISTEN_DOWNRIGHT;
			break;
		case WALK_DOWN:
		case STOP_DOWN:
		case WALK_DOWNLEFT:
		case STOP_DOWNLEFT:
		case TALK_DOWNLEFT:
		case LISTEN_DOWNLEFT:
			newDir = LISTEN_DOWNLEFT;
			break;
		case WALK_LEFT:
		case STOP_LEFT:
		case TALK_LEFT:
		case LISTEN_LEFT:
			newDir = LISTEN_LEFT;
			break;
		case WALK_UPLEFT:
		case STOP_UPLEFT:
		case TALK_UPLEFT:
		case LISTEN_UPLEFT:
			newDir = LISTEN_UPLEFT;
			break;

		default:
			break;
		}

		// See if the NPC's Seq has to wait for an Abort Talk Code
		if (person.hasAborts()) {
			person._gotoSeq = newDir;
		}  else {
			if (person._seqTo) {
				// Reset to previous value
				person._walkSequences[person._sequenceNumber]._sequences[person._frameNumber] = person._seqTo;
				person._seqTo = 0;
			}

			person._sequenceNumber = newDir;
			person._frameNumber = 0;
			person.checkWalkGraphics();
		}
	}
}

void TattooPeople::setTalkSequence(int speaker, int sequenceNum) {
	People &people = *_vm->_people;
	Scene &scene = *_vm->_scene;
	TattooTalk &talk = *(TattooTalk *)_vm->_talk;

	// If no speaker is specified, then nothing needs to be done
	if (speaker == -1)
		return;

	int objNum = people.findSpeaker(speaker);
	if (objNum != -1 && objNum < 256) {
		Object &obj = scene._bgShapes[objNum];

		// See if the Object has to wait for an Abort Talk Code
		if (obj.hasAborts()) {
			talk.pushTalkSequence(&obj);
			obj._gotoSeq = sequenceNum;
		}
		else {
			obj.setObjTalkSequence(sequenceNum);
		}
	}
	else if (objNum != -1) {
		objNum -= 256;
		Person &person = people[objNum];
		int newDir = person._sequenceNumber;

		switch (newDir) {
		case WALK_UP:
		case STOP_UP:
		case WALK_UPRIGHT:
		case STOP_UPRIGHT:
		case TALK_UPRIGHT:
		case LISTEN_UPRIGHT:
			newDir = TALK_UPRIGHT;
			break;
		case WALK_RIGHT:
		case STOP_RIGHT:
		case TALK_RIGHT:
		case LISTEN_RIGHT:
			newDir = TALK_RIGHT;
			break;
		case WALK_DOWNRIGHT:
		case STOP_DOWNRIGHT:
		case TALK_DOWNRIGHT:
		case LISTEN_DOWNRIGHT:
			newDir = TALK_DOWNRIGHT;
			break;
		case WALK_DOWN:
		case STOP_DOWN:
		case WALK_DOWNLEFT:
		case STOP_DOWNLEFT:
		case TALK_DOWNLEFT:
		case LISTEN_DOWNLEFT:
			newDir = TALK_DOWNLEFT;
			break;
		case WALK_LEFT:
		case STOP_LEFT:
		case TALK_LEFT:
		case LISTEN_LEFT:
			newDir = TALK_LEFT;
			break;
		case WALK_UPLEFT:
		case STOP_UPLEFT:
		case TALK_UPLEFT:
		case LISTEN_UPLEFT:
			newDir = TALK_UPLEFT;
			break;
		default:
			break;
		}

		// See if the NPC's sequence has to wait for an Abort Talk Code
		if (person.hasAborts()) {
			person._gotoSeq = newDir;
		} else {
			if (person._seqTo) {
				// Reset to previous value
				person._walkSequences[person._sequenceNumber]._sequences[person._frameNumber] = person._seqTo;
				person._seqTo = 0;
			}

			person._sequenceNumber = newDir;
			person._frameNumber = 0;
			person.checkWalkGraphics();
		}
	}
}


int TattooPeople::findSpeaker(int speaker) {
	int result = People::findSpeaker(speaker);
	const char *portrait = _characters[speaker]._portrait;

	// Fallback that Rose Tattoo uses if no speaker was found
	if (result == -1) {
		bool flag = _vm->readFlags(76);

		if (_data[HOLMES]->_type == CHARACTER && ((speaker == 0 && flag) || (speaker == 1 && !flag)))
			return -1;

		for (uint idx = 1; idx < _data.size(); ++idx) {
			TattooPerson &p = (*this)[idx];

			if (p._type == CHARACTER) {
				Common::String name(p._name.c_str(), p._name.c_str() + 4);

				if (name.equalsIgnoreCase(portrait) && p._npcName[4] >= '0' && p._npcName[4] <= '9')
					return idx + 256;
			}
		}
	}

	return -1;
}

void TattooPeople::synchronize(Serializer &s) {
	s.syncAsByte(_holmesOn);

	for (uint idx = 0; idx < _data.size(); ++idx) {
		Person &p = *_data[idx];
		s.syncAsSint32LE(p._position.x);
		s.syncAsSint32LE(p._position.y);
		s.syncAsSint16LE(p._sequenceNumber);
		s.syncAsSint16LE(p._type);
		s.syncString(p._walkVGSName);
		s.syncString(p._description);
		s.syncString(p._examine);
	}

	s.syncAsSint16LE(_holmesQuotient);

	if (s.isLoading()) {
		_hSavedPos = _data[HOLMES]->_position;
		_hSavedFacing = _data[HOLMES]->_sequenceNumber;
	}
}

bool TattooPeople::loadWalk() {
	Resources &res = *_vm->_res;
	bool result = false;

	for (int idx = 0; idx < MAX_CHARACTERS; ++idx) {
		Person &person = *_data[idx];

		if (!person._walkLoaded && (person._type == CHARACTER || person._type == HIDDEN_CHARACTER)) {
			if (person._type == HIDDEN_CHARACTER)
				person._type = INVALID;

			// See if this is one of the more used Walk Graphics stored in WALK.LIB
			for (int libNum = 0; libNum < NUM_IN_WALK_LIB; ++libNum) {
				if (!person._walkVGSName.compareToIgnoreCase(WALK_LIB_NAMES[libNum])) {
					_useWalkLib = true;
					break;
				}
			}

			// Load the images for the character
			person._images = new ImageFile(person._walkVGSName, false);
			person._maxFrames = person._images->size();

			// Load walk sequence data
			Common::String fname = Common::String(person._walkVGSName.c_str(), strchr(person._walkVGSName.c_str(), '.'));
			fname += ".SEQ";

			// Load the walk sequence data
			Common::SeekableReadStream *stream = res.load(fname, _useWalkLib ? "walk.lib" : "vgs.lib");
				
			person._walkSequences.resize(stream->readByte());

			for (uint seqNum = 0; seqNum < person._walkSequences.size(); ++seqNum)
				person._walkSequences[seqNum].load(*stream);

			// Close the sequences resource
			delete stream;
			_useWalkLib = false;

			person._sequences = &person._walkSequences[person._sequenceNumber]._sequences[0];
			person._seqSize = person._walkSequences[person._sequenceNumber]._sequences.size();
			person._frameNumber = 0;
			person.setImageFrame();

			// Set the stop Frames pointers
			for (int dirNum = 0; dirNum < 8; ++dirNum) {
				int count = 0;
				while (person._walkSequences[dirNum + 8][count] != 0)
					++count;
				count += 2;
				count = person._walkSequences[dirNum + 8][count] - 1;
				person._stopFrames[dirNum] = &(*person._images)[count];
			}

			result = true;
			person._walkLoaded = true;
		} else if (person._type != CHARACTER) {
			person._walkLoaded = false;
		}
	}

	_forceWalkReload = false;
	return result;
}

} // End of namespace Tattoo

} // End of namespace Sherlock