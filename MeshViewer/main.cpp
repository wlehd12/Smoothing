#include <stdio.h>
#include <stdlib.h>
#include "GL\glut.h"
#include "Mesh.h"

float _zoom = 15.0f; // ȭ�� Ȯ��,���
float _rot_x = 0.0f; // x�� ȸ��
float _rot_y = 0.001f; // y�� ȸ��
float _trans_x = 0.0f; // x�� �̵�
float _trans_y = 0.0f; // y�� �̵�
int _last_x = 0; // ���� ���콺 Ŭ�� x��ġ
int _last_y = 0; // ���� ���콺 Ŭ�� y��ġ
unsigned char _buttons[3] = { 0 }; // ���콺 ����(����,������,�� ��ư)
bool simulation = false;
bool force = false;

Mesh* mesh = NULL;
int _renderMode = 3;

void Init(void)
{
	// ���̰� ��� ����
	glEnable(GL_DEPTH_TEST);
}

void Draw(void)
{
	glEnable(GL_LIGHTING); // ���� Ȱ��ȭ
	glEnable(GL_LIGHT0); // ù��° ����

	glBegin(GL_QUADS);
	glVertex3f(10.0, -10.1, 10.0);
	glVertex3f(10.0, -10.1, -10.0);
	glVertex3f(-10.0, -10.1, -10.0);
	glVertex3f(-10.0, -10., 10.0);
	glEnd();

	switch (_renderMode) {
	case 0:
		mesh->drawPoint();
		break;
	case 1:
		mesh->drawWireframe();
		break;
	case 2:
		mesh->drawSurface();
		break;
	case 3:
		mesh->drawSurface(true);
		break;
	}
	glDisable(GL_LIGHTING);
}

void Update(void)
{
	if (simulation) {
		mesh->Simulation(0.01);
		if (force) {
			mesh->Force(0.01);
		}
	}
	mesh->Collision();
	::glutPostRedisplay();
}


void Display(void)
{
	glClearColor(0.6f, 0.6f, 0.6f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	
	glTranslatef(0, 0, -_zoom);
	glTranslatef(_trans_x, _trans_y, 0);
	glRotatef(_rot_x, 1, 0, 0);
	glRotatef(_rot_y, 0, 1, 0);

	Draw(); // �׷����� ��ü
	glutSwapBuffers(); // GLUT_DOUBLE 
}

void Reshape(int w, int h)
{
	if (w == 0) {
		h = 1;
	}
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (float)w / h, 0.1, 100);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void Keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 'Q':
	case 'q':
		exit(0);
	case 'p':
	case 'P':
		_renderMode = 0;
		break;
	case 'w':
	case 'W':
		_renderMode = 1;
		break;
	case 's':
	case 'S':
		_renderMode = 2;
		break;
	case 'a':
	case 'A':
		_renderMode = 3;
		break;
	case 'r':
	case 'R':
		mesh->reset();
		break;
	case 'f':
	case 'F':
		force = !force;
		break;
	case ' ':
		simulation = !simulation;
		break;
	default:
		break;
	}
	glutPostRedisplay(); 
}

void Mouse(int button, int state, int x, int y)
{
	// ���� ���콺 Ŭ�� ��ġ
	_last_x = x;
	_last_y = y;

	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		_buttons[0] = state == GLUT_DOWN ? 1 : 0;
		break;
	case GLUT_MIDDLE_BUTTON:
		_buttons[1] = state == GLUT_DOWN ? 1 : 0;
		break;
	case GLUT_RIGHT_BUTTON:
		_buttons[2] = state == GLUT_DOWN ? 1 : 0;
		break;
	default:
		break;
	}
	glutPostRedisplay(); // ȭ�鰻��
}

void Motion(int x, int y)
{
	int diff_x = x - _last_x;
	int diff_y = y - _last_y;
	_last_x = x;
	_last_y = y;

	if (_buttons[2]) {
		_zoom -= (float)0.02f * diff_x;
	}
	else if (_buttons[1]) {
		_trans_x += (float)0.02f * diff_x;
		_trans_y -= (float)0.02f * diff_y;
	}
	else if (_buttons[0]) {
		_rot_x += (float)0.2f * diff_y;
		_rot_y += (float)0.2f * diff_x;
	}
	glutPostRedisplay(); // ȭ�鰻��
}

void main(int argc, char** argv)
{
	mesh = new Mesh("OBJ\\bunny.obj");
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(800, 800);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Fighting!");
	glutDisplayFunc(Display); // ȭ�鿡 ������ �κ�
	glutReshapeFunc(Reshape); // ȭ��â �ʱ�ȭ, â�� ũ�� ����
	glutIdleFunc(Update);
	glutMouseFunc(Mouse); // ���콺 Ŭ�� �̺�Ʈ	
	glutMotionFunc(Motion); // ���콺 �̵� �̺�Ʈ
	glutKeyboardFunc(Keyboard); // Ű���� Ŭ�� �̺�Ʈ
	Init(); // ���� Ŭ����, ���� �ʱ�ȭ
	glutMainLoop();
}