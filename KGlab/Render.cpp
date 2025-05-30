#include "Render.h"
#include <Windows.h>
#include <GL\GL.h>
#include <GL\GLU.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include "GUItextRectangle.h"
#define PI 3.14





#ifdef _DEBUG
#include <Debugapi.h> 
struct debug_print
{
	template<class C>
	debug_print& operator<<(const C& a)
	{
		OutputDebugStringA((std::stringstream() << a).str().c_str());
		return *this;
	}
} debout;
#else
struct debug_print
{
	template<class C>
	debug_print& operator<<(const C& a)
	{
		return *this;
	}
} debout;
#endif

//библиотека для разгрузки изображений
//https://github.com/nothings/stb
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//внутренняя логика "движка"
#include "MyOGL.h"
extern OpenGL gl;
#include "Light.h"
Light light;
#include "Camera.h"
Camera camera;


bool texturing = true;
bool lightning = true;
bool alpha = false;


//переключение режимов освещения, текстурирования, альфаналожения
void switchModes(OpenGL* sender, KeyEventArg arg)
{
	//конвертируем код клавиши в букву
	auto key = LOWORD(MapVirtualKeyA(arg.key, MAPVK_VK_TO_CHAR));

	switch (key)
	{
	case 'L':
		lightning = !lightning;
		break;
	case 'T':
		texturing = !texturing;
		break;
	case 'A':
		alpha = !alpha;
		break;
	}
}

//Текстовый прямоугольничек в верхнем правом углу.
//OGL не предоставляет возможности для хранения текста
//внутри этого класса создается картинка с текстом (через виндовый GDI),
//в виде текстуры накладывается на прямоугольник и рисуется на экране.
//Это самый простой способ что то написать на экране
//но ооооочень не оптимальный
GuiTextRectangle text;

//айдишник для текстуры
GLuint texId;
//выполняется один раз перед первым рендером
void initRender()
{
	//==============НАСТРОЙКА ТЕКСТУР================
	//4 байта на хранение пикселя
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	//просим сгенерировать нам Id для текстуры
	//и положить его в texId
	glGenTextures(1, &texId);

	//делаем текущую текстуру активной
	//все, что ниже будет применено texId текстуре.
	glBindTexture(GL_TEXTURE_2D, texId);


	int x, y, n;

	//загружаем картинку
	//см. #include "stb_image.h" 
	unsigned char* data = stbi_load("kostya.jpg", &x, &y, &n, 4);
	//x - ширина изображения
	//y - высота изображения
	//n - количество каналов
	//4 - нужное нам количество каналов
	//пиксели будут хранится в памяти [R-G-B-A]-[R-G-B-A]-[..... 
	// по 4 байта на пиксель - по байту на канал
	//пустые каналы будут равны 255

	//Картинка хранится в памяти перевернутой 
	//так как ее начало в левом верхнем углу
	//по этому мы ее переворачиваем -
	//меняем первую строку с последней,
	//вторую с предпоследней, и.т.д.
	unsigned char* _tmp = new unsigned char[x * 4]; //времянка
	for (int i = 0; i < y / 2; ++i)
	{
		std::memcpy(_tmp, data + i * x * 4, x * 4);//переносим строку i в времянку
		std::memcpy(data + i * x * 4, data + (y - 1 - i) * x * 4, x * 4); //(y-1-i)я строка -> iя строка
		std::memcpy(data + (y - 1 - i) * x * 4, _tmp, x * 4); //времянка -> (y-1-i)я строка
	}
	delete[] _tmp;


	//загрузка изображения в видеопамять
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	//выгрузка изображения из опперативной памяти
	stbi_image_free(data);


	//настройка режима наложения текстур
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	//GL_REPLACE -- полная замена политога текстурой
//настройка тайлинга
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	//настройка фильтрации
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//======================================================

	//================НАСТРОЙКА КАМЕРЫ======================
	camera.caclulateCameraPos();

	//привязываем камеру к событиям "движка"
	gl.WheelEvent.reaction(&camera, &Camera::Zoom);
	gl.MouseMovieEvent.reaction(&camera, &Camera::MouseMovie);
	gl.MouseLeaveEvent.reaction(&camera, &Camera::MouseLeave);
	gl.MouseLdownEvent.reaction(&camera, &Camera::MouseStartDrag);
	gl.MouseLupEvent.reaction(&camera, &Camera::MouseStopDrag);
	//==============НАСТРОЙКА СВЕТА===========================
	//привязываем свет к событиям "движка"
	gl.MouseMovieEvent.reaction(&light, &Light::MoveLight);
	gl.KeyDownEvent.reaction(&light, &Light::StartDrug);
	gl.KeyUpEvent.reaction(&light, &Light::StopDrug);
	//========================================================
	//====================Прочее==============================
	gl.KeyDownEvent.reaction(switchModes);
	text.setSize(512, 180);
	//========================================================


	camera.setPosition(2, 1.5, 1.5);
}

void calculateNormal(double* v1, double* v2, double* v3, double* normal) {
	double vec1[3], vec2[3];

	// Вычисляем два вектора, лежащих на плоскости
	vec1[0] = v2[0] - v1[0];
	vec1[1] = v2[1] - v1[1];
	vec1[2] = v2[2] - v1[2];

	vec2[0] = v3[0] - v1[0];
	vec2[1] = v3[1] - v1[1];
	vec2[2] = v3[2] - v1[2];

	// Вычисляем векторное произведение vec1 и vec2
	normal[0] = vec1[1] * vec2[2] - vec1[2] * vec2[1];
	normal[1] = vec1[2] * vec2[0] - vec1[0] * vec2[2];
	normal[2] = vec1[0] * vec2[1] - vec1[1] * vec2[0];

	// Нормализуем вектор нормали
	double length = sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
	normal[0] /= length;
	normal[1] /= length;
	normal[2] /= length;
}

void Render(double delta_time)
{
	glEnable(GL_DEPTH_TEST);

	//натройка камеры и света
	//в этих функциях находятся OGLные функции
	//которые устанавливают параметры источника света
	//и моделвью матрицу, связанные с камерой.

	if (gl.isKeyPressed('F')) //если нажата F - свет из камеры
	{
		light.SetPosition(camera.x(), camera.y(), camera.z());
	}
	camera.SetUpCamera();
	light.SetUpLight();


	//рисуем оси
	gl.DrawAxes();

	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);


	//включаем режимы, в зависимости от нажания клавиш. см void switchModes(OpenGL *sender, KeyEventArg arg)
	if (lightning)
		glEnable(GL_LIGHTING);
	if (texturing)
	{
		glEnable(GL_TEXTURE_2D);
		glColor3f(0.0, 1.0, 0); // Зеленоватый оттенок

		double start[]{ 0, 0, 0 };
		double A[]{ 0, -1, 0 };
		double A1[]{ 0, -1, 5 };

		double B[]{ -2, -3.5, 0 };
		double B1[]{ -2, -3.5, 5 };

		double C[]{ -0.5, 0, 0 };
		double C1[]{ -0.5, 0, 5 };

		double D[]{ -2, 1.5, 0 };
		double D1[]{ -2, 1.5, 5 };

		double E[]{ -1, 4, 0 };
		double E1[]{ -1, 4, 5 };

		double F[]{ 1.5, 3.5, 0 };
		double F1[]{ 1.5, 3.5, 5 };

		double G[]{ 0.5, 0, 0 };
		double G1[]{ 0.5, 0, 5 };

		double H[]{ 3, -2, 0 };
		double H1[]{ 3, -2, 5 };
		double normal[3];
		double start1[]{ 0, 0, 1 };



		glBindTexture(GL_TEXTURE_2D, texId);

		glBegin(GL_QUADS);

		// Грань A1, B1, C1
		glNormal3d(0, 0, 1);
		glTexCoord2f(0.4, 0.333); glVertex3dv(A1);
		glTexCoord2f(0.0, 0.0);   glVertex3dv(B1);
		glTexCoord2f(0.3, 0.467); glVertex3dv(C1);

		// Грань C1, D1, E1
		glNormal3d(0, 0, 1);
		glTexCoord2f(0.3, 0.467); glVertex3dv(C1);
		glTexCoord2f(0.0, 0.667); glVertex3dv(D1);
		glTexCoord2f(0.2, 1.0);   glVertex3dv(E1);

		// Грань E1, F1, G1
		glNormal3d(0, 0, 1);
		glTexCoord2f(0.2, 1.0);   glVertex3dv(E1);
		glTexCoord2f(0.7, 0.933); glVertex3dv(F1);
		glTexCoord2f(0.5, 0.467); glVertex3dv(G1);

		// Грань G1, H1, A1
		glNormal3d(0, 0, 1);
		glTexCoord2f(0.5, 0.467); glVertex3dv(G1);
		glTexCoord2f(1.0, 0.2);   glVertex3dv(H1);
		glTexCoord2f(0.4, 0.333); glVertex3dv(A1);

		// Грань C1, G1, E1
		glNormal3d(0, 0, 1);
		glTexCoord2f(0.3, 0.467); glVertex3dv(C1);
		glTexCoord2f(0.5, 0.467); glVertex3dv(G1);
		glTexCoord2f(0.2, 1.0);   glVertex3dv(E1);

		// Грань C1, G1, A1
		glNormal3d(0, 0, 1);
		glTexCoord2f(0.3, 0.467); glVertex3dv(C1);
		glTexCoord2f(0.5, 0.467); glVertex3dv(G1);
		glTexCoord2f(0.4, 0.333); glVertex3dv(A1);



		glBindTexture(GL_TEXTURE_2D, 0); //сбрасываем текущую текстуру
	}

	if (alpha)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);

		// Отрисовка прозрачных объектов
		// (ваш код для отрисовки прозрачных объектов)

		// Включаем запись в буфер глубины обратно
		glDepthMask(GL_TRUE);
	}

	//=============НАСТРОЙКА МАТЕРИАЛА==============


	//настройка материала, все что рисуется ниже будет иметь этот метериал.
	//массивы с настройками материала
	float  amb[] = { 0.2, 0.2, 0.1, 1. };
	float dif[] = { 0.4, 0.65, 0.5, 1. };
	float spec[] = { 0.9, 0.8, 0.3, 1. };
	float sh = 0.2f * 256;

	//фоновая
	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	//дифузная
	glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
	//зеркальная
	glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
	//размер блика
	glMaterialf(GL_FRONT, GL_SHININESS, sh);

	//чтоб было красиво, без квадратиков (сглаживание освещения)
	glShadeModel(GL_SMOOTH); //закраска по Гуро      
	//(GL_SMOOTH - плоская закраска)

//============ РИСОВАТЬ ТУТ ==============

	double A[]{ 0, -1, 0 };
	double A1[]{ 0, -1, 5 };

	double B[]{ -2, -3.5, 0 };
	double B1[]{ -2, -3.5, 5 };

	double C[]{ -0.5, 0, 0 };
	double C1[]{ -0.5, 0, 5 };

	double D[]{ -2, 1.5, 0 };
	double D1[]{ -2, 1.5, 5 };

	double E[]{ -1, 4, 0 };
	double E1[]{ -1, 4, 5 };

	double F[]{ 1.5, 3.5, 0 };
	double F1[]{ 1.5, 3.5, 5 };

	double G[]{ 0.5, 0, 0 };
	double G1[]{ 0.5, 0, 5 };

	double H[]{ 3, -2, 0 };
	double H1[]{ 3, -2, 5 };
	double normal[3];



	// Включение текстуры
	//glEnable(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, texId); // texId — идентификатор загруженной текстуры

	glBegin(GL_TRIANGLES);

	// Грань A, B, C
	glNormal3d(0, 0, -1);
	glTexCoord2f(0.4, 0.333); glVertex3dv(A);
	glTexCoord2f(0.0, 0.0);   glVertex3dv(B);
	glTexCoord2f(0.3, 0.467); glVertex3dv(C);

	// Грань C, D, E
	glNormal3d(0, 0, -1);
	glTexCoord2f(0.3, 0.467); glVertex3dv(C);
	glTexCoord2f(0.0, 0.667); glVertex3dv(D);
	glTexCoord2f(0.2, 1.0);   glVertex3dv(E);

	// Грань E, F, G
	glNormal3d(0, 0, -1);
	glTexCoord2f(0.2, 1.0);   glVertex3dv(E);
	glTexCoord2f(0.7, 0.933); glVertex3dv(F);
	glTexCoord2f(0.5, 0.467); glVertex3dv(G);

	// Грань G, H, A
	glNormal3d(0, 0, -1);
	glTexCoord2f(0.5, 0.467); glVertex3dv(G);
	glTexCoord2f(1.0, 0.2);   glVertex3dv(H);
	glTexCoord2f(0.4, 0.333); glVertex3dv(A);

	// Грань C, G, E
	glNormal3d(0, 0, -1);
	glTexCoord2f(0.3, 0.467); glVertex3dv(C);
	glTexCoord2f(0.5, 0.467); glVertex3dv(G);
	glTexCoord2f(0.2, 1.0);   glVertex3dv(E);

	// Грань C, G, A
	glNormal3d(0, 0, -1);
	glTexCoord2f(0.3, 0.467); glVertex3dv(C);
	glTexCoord2f(0.5, 0.467); glVertex3dv(G);
	glTexCoord2f(0.4, 0.333); glVertex3dv(A);

	// Грань A1, B1, C1
	glNormal3d(0, 0, 1);
	glTexCoord2f(0.4, 0.333); glVertex3dv(A1);
	glTexCoord2f(0.0, 0.0);   glVertex3dv(B1);
	glTexCoord2f(0.3, 0.467); glVertex3dv(C1);

	// Грань C1, D1, E1
	glNormal3d(0, 0, 1);
	glTexCoord2f(0.3, 0.467); glVertex3dv(C1);
	glTexCoord2f(0.0, 0.667); glVertex3dv(D1);
	glTexCoord2f(0.2, 1.0);   glVertex3dv(E1);

	// Грань E1, F1, G1
	glNormal3d(0, 0, 1);
	glTexCoord2f(0.2, 1.0);   glVertex3dv(E1);
	glTexCoord2f(0.7, 0.933); glVertex3dv(F1);
	glTexCoord2f(0.5, 0.467); glVertex3dv(G1);

	// Грань G1, H1, A1
	glNormal3d(0, 0, 1);
	glTexCoord2f(0.5, 0.467); glVertex3dv(G1);
	glTexCoord2f(1.0, 0.2);   glVertex3dv(H1);
	glTexCoord2f(0.4, 0.333); glVertex3dv(A1);

	// Грань C1, G1, E1
	glNormal3d(0, 0, 1);
	glTexCoord2f(0.3, 0.467); glVertex3dv(C1);
	glTexCoord2f(0.5, 0.467); glVertex3dv(G1);
	glTexCoord2f(0.2, 1.0);   glVertex3dv(E1);

	// Грань C1, G1, A1
	glNormal3d(0, 0, 1);
	glTexCoord2f(0.3, 0.467); glVertex3dv(C1);
	glTexCoord2f(0.5, 0.467); glVertex3dv(G1);
	glTexCoord2f(0.4, 0.333); glVertex3dv(A1);

	glEnd();

	// Отключение текстуры
	/*glBindTexture(GL_TEXTURE_2D, 0);*/

	glBegin(GL_QUADS);

	// Боковая грань A, B, B1, A1 1
	calculateNormal(A, B, B1, normal);

	glColor3d(0.5, 0.5, 0.0);
	glNormal3dv(normal);
	glVertex3dv(A);
	glVertex3dv(B);
	glVertex3dv(B1);
	glVertex3dv(A1);

	// Боковая грань B, C, C1, B1 2
	calculateNormal(B, C, C1, normal);
	glColor3d(1.0, 0.4, 0.7);
	glNormal3dv(normal);
	glVertex3dv(B);
	glVertex3dv(C);
	glVertex3dv(C1);
	glVertex3dv(B1);

	// Боковая грань C, D, D1, C1 3
	calculateNormal(C, D, D1, normal);
	glColor3d(0.6, 0.3, 0.1);
	glNormal3dv(normal);
	glVertex3dv(C);
	glVertex3dv(D);
	glVertex3dv(D1);
	glVertex3dv(C1);

	// Боковая грань D, E, E1, D1 4
	calculateNormal(D, E, E1, normal);
	glColor3d(1.0, 0.6, 0.4);
	glNormal3dv(normal);
	glVertex3dv(D);
	glVertex3dv(E);
	glVertex3dv(E1);
	glVertex3dv(D1);

	// Боковая грань E, F, F1, E1 5
	calculateNormal(E, F, F1, normal);
	glColor3d(0.0, 0.1, 0.5);
	glNormal3dv(normal);
	glVertex3dv(E);
	glVertex3dv(F);
	glVertex3dv(F1);
	glVertex3dv(E1);

	// Боковая грань F, G, G1, F1 6
	calculateNormal(F, G, G1, normal);
	glColor3d(0.7, 1.0, 0.0);
	glNormal3dv(normal);
	glVertex3dv(F);
	glVertex3dv(G);
	glVertex3dv(G1);
	glVertex3dv(F1);

	// Боковая грань G, H, H1, G1 7
	calculateNormal(G, H, H1, normal);
	glColor3d(0.5, 0.0, 0.2);
	glVertex3dv(G);
	glVertex3dv(H);
	glVertex3dv(H1);
	glVertex3dv(G1);

	// Боковая грань H, A, A1, H1 8
	calculateNormal(H, A, A1, normal);
	glColor3d(0.6, 0.6, 0.9);
	glNormal3dv(normal);
	glVertex3dv(H);
	glVertex3dv(A);
	glVertex3dv(A1);
	glVertex3dv(H1);

	glEnd();



	//===============================================

	//рисуем источник света
	light.DrawLightGizmo();

	//================Сообщение в верхнем левом углу=======================

	//переключаемся на матрицу проекции
	glMatrixMode(GL_PROJECTION);
	//сохраняем текущую матрицу проекции с перспективным преобразованием
	glPushMatrix();
	//загружаем единичную матрицу в матрицу проекции
	glLoadIdentity();

	//устанавливаем матрицу паралельной проекции
	glOrtho(0, gl.getWidth() - 1, 0, gl.getHeight() - 1, 0, 1);

	//переключаемся на моделвью матрицу
	glMatrixMode(GL_MODELVIEW);
	//сохраняем матрицу
	glPushMatrix();
	//сбразываем все трансформации и настройки камеры загрузкой единичной матрицы
	glLoadIdentity();

	//отрисованное тут будет визуалзироватся в 2д системе координат
	//нижний левый угол окна - точка (0,0)
	//верхний правый угол (ширина_окна - 1, высота_окна - 1)


	std::wstringstream ss;
	ss << std::fixed << std::setprecision(3);
	ss << "T - " << (texturing ? L"[вкл]выкл  " : L" вкл[выкл] ") << L"текстур" << std::endl;
	ss << "L - " << (lightning ? L"[вкл]выкл  " : L" вкл[выкл] ") << L"освещение" << std::endl;
	ss << "A - " << (alpha ? L"[вкл]выкл  " : L" вкл[выкл] ") << L"альфа-наложение" << std::endl;
	ss << L"F - Свет из камеры" << std::endl;
	ss << L"G - двигать свет по горизонтали" << std::endl;
	ss << L"G+ЛКМ двигать свет по вертекали" << std::endl;
	ss << L"Коорд. света: (" << std::setw(7) << light.x() << "," << std::setw(7) << light.y() << "," << std::setw(7) << light.z() << ")" << std::endl;
	ss << L"Коорд. камеры: (" << std::setw(7) << camera.x() << "," << std::setw(7) << camera.y() << "," << std::setw(7) << camera.z() << ")" << std::endl;
	ss << L"Параметры камеры: R=" << std::setw(7) << camera.distance() << ",fi1=" << std::setw(7) << camera.fi1() << ",fi2=" << std::setw(7) << camera.fi2() << std::endl;
	ss << L"delta_time: " << std::setprecision(5) << delta_time << std::endl;


	text.setPosition(10, gl.getHeight() - 10 - 180);
	text.setText(ss.str().c_str());
	text.Draw();

	//восстанавливаем матрицу проекции на перспективу, которую сохраняли ранее.
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();


}


