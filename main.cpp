#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include "wx/wx.h"
#include "wx/sizer.h"
#include <vector>
#include <string>
#include <algorithm>


#define SOLIT_GAME_BOARD_SIZE 7
#define SOLIT_GFX_OFFSET 200


#define SOLIT_COLOR_COUNT 4
#define SOLIT_COLOR_INVALID 0
#define SOLIT_COLOR_EMPTY 0xFFFFFF


class solit_game_slot {
public:
	//there are 4 * 4 slots on board that are invalid, in the corners
	int color; //color, if flag empty is set
	char id;
	wxRect rect;
};

struct solit_pos
{
	int column;
	int row;
	solit_pos(int row, int column) { this->column = column; this->row = row; }
};

class solit_game {
private:
	std::vector<solit_game_slot*> _game_field;
	solit_game_slot* _selected_slot;
	int kill_count;
	int _colors[SOLIT_COLOR_COUNT] = { 0x000000B3,0x880000,0x00FFFF,0x66FF66 };

	int gen_random_color();
public:


	int get_kill_count() const
	{
		return kill_count;
	}

	void set_kill_count(int kill_count)
	{
		this->kill_count = kill_count;
	}


	void set_selected_slot(solit_game_slot* solit_game_slot)
	{
		_selected_slot = solit_game_slot;
	}

	solit_game_slot* get_selected_slot() const
	{
		return _selected_slot;
	}



	solit_game();
	virtual ~solit_game();

	solit_game_slot* solit_game::get_slot(solit_pos& pos);
	solit_game_slot* get_slot(wxPoint p); //by x,y coords
	solit_game_slot* get_slot(int index); //by index, starting at top left with 0
	void init_game_field(); //setup game field
	int move(solit_game_slot* src, solit_game_slot* dst);

};

int solit_game::move(solit_game_slot * src, solit_game_slot* dst)
{

	//ckeck if the movement would be legal, geometry wise
	int target_dif = dst->id - src->id;

	//same slot?
	if (!target_dif)
		return false;

	//invalid target slot?
	if (!(dst->color != SOLIT_COLOR_INVALID))
		return false;

	//invalid src slot?
	if ((src->color == SOLIT_COLOR_INVALID) || (src->color == SOLIT_COLOR_EMPTY))
		return false;

	//check distances. Can either be 2 or 14 (abs)
	if ((abs(target_dif) != 2) && (abs(target_dif) != 14))
		return false;

	//figure out jumped rows and cols
	int diff_row = (dst->id / SOLIT_GAME_BOARD_SIZE) - (src->id / SOLIT_GAME_BOARD_SIZE);
	int diff_col = target_dif % SOLIT_GAME_BOARD_SIZE;

	//only one of these difs can be nonzero, otherwise player did a geometrically impossible move using bounds of board
	//use xor
	if (!((diff_row != 0) ^ (diff_col != 0)))
		return false;

	//check if we have a piece in the various directions we can jump over. 
	//also, the jump target must be empty.
	//get jumped over piece
	//src + target_dif / 2 always yields jumped over piece
	solit_game_slot* target_slot_jumpover = get_slot(src->id + (target_dif / 2));
	//src + target dif always yields dest

	if (target_slot_jumpover)
	{
		if ((target_slot_jumpover->color != SOLIT_COLOR_INVALID) && !(target_slot_jumpover->color == SOLIT_COLOR_EMPTY))
		{
			if  (dst->color == SOLIT_COLOR_EMPTY)
			{
				//jump slot is valid and has a piece on it
				//nil this one as removed
				target_slot_jumpover->color = SOLIT_COLOR_EMPTY;
				dst->color = src->color;
				src->color = SOLIT_COLOR_EMPTY;
				return true;
			}
		} return false;
	}
	
	return false; //should never happen



}


class DrawPlaneSolit : public wxPanel
{
private:
	solit_game* _game;
	float _scale;
	wxTimer _timer;
	int _ctr;
public:

	DrawPlaneSolit(wxFrame* parent, solit_game* game);

	void paintEvent(wxPaintEvent & evt);
	void paintNow();

	void render(wxDC& dc);

	void mouseDown(wxMouseEvent& event);
	void keyPressed(wxKeyEvent& event);
	void event_timer(wxTimerEvent& event);



	DECLARE_EVENT_TABLE()
};


DrawPlaneSolit::DrawPlaneSolit(wxFrame* parent, solit_game* game) :
	wxPanel(parent)
{
	_game = game;
	_scale = 1.0f;
	_timer.SetOwner(this);
	_timer.Start(33);

}


void DrawPlaneSolit::paintEvent(wxPaintEvent & evt)
{
	wxPaintDC dc(this);
	render(dc);
}

void DrawPlaneSolit::paintNow()
{
	wxClientDC dc(this);
	render(dc);
}
void DrawPlaneSolit::render(wxDC&  dc)
{

	// draw some text
	dc.Clear();
	for (int y = 0; y < SOLIT_GAME_BOARD_SIZE; y++) {
		for (int x = 0; x < SOLIT_GAME_BOARD_SIZE; x++) {
			solit_game_slot* slot = _game->get_slot(solit_pos(y, x));

			if (slot->color != SOLIT_COLOR_INVALID)
			{
				if (slot->color == SOLIT_COLOR_EMPTY)
					dc.SetBrush(wxColor(255, 255, 255)); // white filling
				else
					dc.SetBrush(wxColor(slot->color)); // green filling

				slot->rect = wxRect(SOLIT_GFX_OFFSET + x * 50 * _scale, SOLIT_GFX_OFFSET + y * 50 * _scale, 45 * _scale, 45 * _scale);


				if (slot == _game->get_selected_slot()) {
					dc.SetPen(wxPen(wxColor(0, 0, 0), 10 * _scale));
					dc.DrawRectangle(slot->rect);
				}
				else
				{
					dc.SetPen(wxPen(wxColor(0, 0, 0), 5 * _scale));
					dc.DrawRectangle(slot->rect);
				}

				//dc.DrawText(std::to_string(slot->id), slot->rect.GetX() + 5, slot->rect.GetY()+5);
			}


		}
	}
	std::string killcounter = "Killcount : " + std::to_string(_game->get_kill_count());
	dc.DrawText(killcounter, 20, 20);

	std::string rest_counter = "Remaining : " + std::to_string(SOLIT_GAME_BOARD_SIZE * SOLIT_GAME_BOARD_SIZE - _game->get_kill_count() - 16 - 1);
	dc.DrawText(rest_counter, 20, 60);

	dc.DrawText("Restart with 'N', Zoom + und -", 20, 100);
}

void DrawPlaneSolit::mouseDown(wxMouseEvent & event)
{

	wxPoint pos = event.GetPosition();

	solit_game_slot* slot = _game->get_slot(pos);

	if (slot)
	{
		//MessageBoxA(0, std::to_string(slot->id).c_str(), "test", 0);
		//_game->set_selected_slot(slot);
		if (!_game->get_selected_slot())
			_game->set_selected_slot(slot);
		else
		{
			//already selected, so this is second click with other slot
			//can we move here?
			//if ( == SOLIT_MOVE_RES_OK)
			if (_game->move(_game->get_selected_slot(), slot))
			{
				_game->set_kill_count(_game->get_kill_count() + 1);
			}
			_game->set_selected_slot(NULL);
		}
	}

	this->paintNow();

}

void DrawPlaneSolit::keyPressed(wxKeyEvent& event)
{
	if (event.GetKeyCode() == 'N')
	{
		_game->init_game_field();
		paintNow();
	}
	else if (event.GetKeyCode() == '+')
	{
		_scale += .1f;
		paintNow();
	}
	else if (event.GetKeyCode() == '-')
	{
		_scale -= .1f;
		paintNow();
	}

}

void DrawPlaneSolit::event_timer(wxTimerEvent& event)
{
	//wxClientDC dc(this);
	//_ctr++;
	//dc.Clear();
	//dc.DrawRectangle(200,wxMax(sin(_ctr * 0.1f),0) * 300, 50, 50);
}



class solit_gui : public wxApp
{
	bool OnInit();

	solit_game game;
	wxFrame *frame;
	DrawPlaneSolit * drawPane;
public:

};

IMPLEMENT_APP(solit_gui)

bool solit_gui::OnInit()
{

	srand(345345345);
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	frame = new wxFrame((wxFrame *)NULL, -1, wxT("Solit"), wxPoint(50, 50), wxSize(650, 650));

	

	drawPane = new DrawPlaneSolit((wxFrame*)frame, &game);
	sizer->Add(drawPane, 1, wxEXPAND);

	drawPane->Bind(wxEVT_LEFT_DOWN, &DrawPlaneSolit::mouseDown, drawPane);
	drawPane->Bind(wxEVT_KEY_DOWN, &DrawPlaneSolit::keyPressed, drawPane);
	drawPane->Bind(wxEVT_TIMER, &DrawPlaneSolit::event_timer, drawPane);

	frame->SetSizer(sizer);
	frame->SetAutoLayout(true);

	frame->Show();
	return true;
}

BEGIN_EVENT_TABLE(DrawPlaneSolit, wxPanel)

// catch paint events
EVT_PAINT(DrawPlaneSolit::paintEvent)

END_EVENT_TABLE()


int solit_game::gen_random_color()
{
	//return (rand() % 255) | (rand() % 255) << 8 | (rand() % 255) << 16 | (rand() % 255) << 32;
	return _colors[rand() % SOLIT_COLOR_COUNT];
}

solit_game::solit_game()
{
	//allocate game board vectors
	_game_field.resize(SOLIT_GAME_BOARD_SIZE * SOLIT_GAME_BOARD_SIZE);
	for (int i = 0; i < _game_field.size(); i++) {
		_game_field[i] = new solit_game_slot();
		_game_field[i]->color = SOLIT_COLOR_EMPTY;
		_game_field[i]->id = i;
	}

	init_game_field();



}

solit_game::~solit_game()
{
}

solit_game_slot * solit_game::get_slot(solit_pos& pos)
{
	if ((pos.column < 0) || (pos.row< 0))
		return NULL;
	if ((pos.column >= SOLIT_GAME_BOARD_SIZE) || (pos.row >= SOLIT_GAME_BOARD_SIZE))
		return NULL;
	return _game_field[pos.row * SOLIT_GAME_BOARD_SIZE + pos.column];
}

solit_game_slot* solit_game::get_slot(wxPoint p)
{
	for (int x = 0; x < _game_field.size(); x++) {
		if (_game_field[x]->rect.Contains(p))
			return _game_field[x];
	}
	return  NULL;
}

solit_game_slot* solit_game::get_slot(int index)
{
	if (index < 0)
		return NULL;
	if (index >= (SOLIT_GAME_BOARD_SIZE * SOLIT_GAME_BOARD_SIZE))
		return NULL;

	return _game_field[index];
}


void solit_game::init_game_field()
{
	kill_count = 0;

	//init game board slot states
	for (int y = 0; y < SOLIT_GAME_BOARD_SIZE; y++) {
		for (int x = 0; x < SOLIT_GAME_BOARD_SIZE; x++) {
			solit_game_slot* slot = get_slot(solit_pos(y,x));
			slot->color = 0;
			//setup corners
			if ((x >= 2) && (x < 5)) {
				slot->color = gen_random_color();
			}
			if ((y >= 2) && (y < 5)) {
				slot->color = gen_random_color();
			}
		}
	}
	//setup center, valid and empty
	get_slot(solit_pos(3,3))->color = SOLIT_COLOR_EMPTY;
}