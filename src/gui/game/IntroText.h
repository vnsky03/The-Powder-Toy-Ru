#pragma once
#include "Config.h"
#include "SimulationConfig.h"
#include "common/String.h"

inline ByteString VersionInfo()
{
	ByteStringBuilder sb;
	sb << DISPLAY_VERSION[0] << "." << DISPLAY_VERSION[1];
	if constexpr (!SNAPSHOT)
	{
		sb << "." << APP_VERSION.build;
	}
	sb << " " << IDENT;
	if constexpr (MOD)
	{
		sb << " MOD " << MOD_ID << " UPSTREAM " << UPSTREAM_VERSION.build;
	}
	if constexpr (SNAPSHOT)
	{
		sb << " SNAPSHOT " << APP_VERSION.build;
	}
	if constexpr (LUACONSOLE)
	{
		sb << " LUACONSOLE";
	}
	if constexpr (NOHTTP)
	{
		sb << " NOHTTP";
	}
	else if constexpr (ENFORCE_HTTPS)
	{
		sb << " HTTPS";
	}
	if constexpr (DEBUG)
	{
		sb << " DEBUG";
	}
	return sb.Build();
}

inline ByteString IntroText()
{
	ByteStringBuilder sb;
	sb << "\bl\bU" << APPNAME << "\bU — Версия " << DISPLAY_VERSION[0] << "." << DISPLAY_VERSION[1] << " — https://powdertoy.co.uk, irc.libera.chat #powder, https://tpt.io/discord\n"
	      "\n"
	      "\n"
	      "\bgНажмите \bo'F1'\bg, чтобы показать или скрыть этот текст.\n"
	      "\n"
	      "\bgЧтобы выбрать материал, наведите курсор на одну из иконок справа — появится список элементов этой группы.\n"
	      "Выбирайте материал из меню кнопками \boлевая/правая кнопка мыши\bg.\n"
	      "Рисуйте произвольные линии, перетаскивая мышь с зажатой левой/правой кнопкой по полю рисования.\n"
	      "\n"
	      "Используйте \boколесо мыши\bg или \bo'['\bg и \bo']'\bg, чтобы менять размер инструмента для частиц. Нажмите \boTab\bg, чтобы сменить форму кисти.\n"
	      "\boСредний клик\bg или \boAlt+клик\bg — «проба» частиц.\n"
	      "\boCtrl+C/V/X\bg — копирование, вставка и вырезание соответственно.\n"
	      "При вставке используйте \bo'R'\bg для поворота, \boShift+R\bg и \boShift+Ctrl+R\bg для отражения по вертикали или горизонтали.\n"
	      "\boShift+перетаскивание\bg создаёт прямые линии частиц. \boShift+Alt+перетаскивание\bg — горизонтальные, вертикальные и диагональные линии.\n"
	      "\boCtrl+перетаскивание\bg — закрашенные прямоугольники. \boCtrl+Alt+перетаскивание\bg — закрашенные квадраты. \boCtrl+Shift+клик\bg заливает замкнутую область.\n"
	      "\n"
	      "\boПробел\bg — пауза физики. \bo'F'\bg — шаг вперёд на один кадр, \bo'F5'\bg — перезагрузка симуляции.\n"
	      "\boCtrl+Z\bg — отмена, \boCtrl+Y\bg или \boCtrl+Shift+Z\bg — повтор.\n"
	      "Используйте \bo'S'\bg, чтобы сохранять части окна как «штампы». \bo'L'\bg загружает последний штамп, \bo'K'\bg показывает библиотеку ваших штампов.\n"
	      "\n"
	      "Используйте \bo0-9\bg для выбора режима отображения.\n"
	      "Используйте \bo'H'\bg, чтобы включать/выключать HUD. Используйте \bo'D'\bg для отладочного режима в HUD.\n"
	      "Используйте \bo'Z'\bg для инструмента зума. Клик зафиксирует окно зума на месте. Колесо меняет силу увеличения.\n"
	      "Используйте \boCtrl+F\bg, чтобы подсветить выбранный элемент на экране.\n"
	      "\n";
	if constexpr (BETA)
	{
		sb << "\brЭто БЕТА: нельзя публиковать сохранения, а локальные сохранения и штампы, созданные в ней, не откроются в старых версиях.\n"
		      "\brЕсли вы планируете публиковать сохранения, используйте финальную версию.\n";
	}
	else
	{
		sb << "\bgЧтобы пользоваться онлайн-функциями (например, сохранением), нужно зарегистрироваться на: \br" << SERVER << "/Register.html\n";
	}
	sb << "\n\bt" << VersionInfo();
	return sb.Build();
}
