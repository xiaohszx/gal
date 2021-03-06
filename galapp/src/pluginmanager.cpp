﻿#include "plugin.h"
#include "pluginmanager.h"
#include "omniobject.h"

#include <QTimer>
#include <QApplication>
#include <QDir>
#include <QSettings>
#include <QThread>
#include <QKeyEvent>

PluginManager::PluginManager(QObject* parent /*= Q_NULLPTR*/)
	: QObject(parent)
{
	QTimer::singleShot(0, this, &PluginManager::firstInit);
}

PluginManager::~PluginManager()
{
}

void PluginManager::setStackedWidget(PluginStackedWidget* pWidget)
{
	m_stackedWidget = pWidget;
}


bool PluginManager::isPluginExist(const QString& key)
{
	Plugin* p = m_plugins.value(key);
	return p != Q_NULLPTR;
}

Plugin* PluginManager::queryPlugin(const QString& key) const
{
	Plugin* p = m_plugins.value(key);
	return p;
}

void PluginManager::onStartQuery(const QString& key, const QString& value)
{
	Plugin* p = m_plugins.value(key);
	if (!p)
		return;

	execQuery(p, value);
}

void PluginManager::firstInit()
{
// 	m_workThread = new QThread(this);
// 	moveToThread(m_workThread);
// 	m_workThread->start();

	init();
}

void PluginManager::init()
{
	//scan the plugin
	QString pluginsDir = QDir::toNativeSeparators(qApp->applicationDirPath() + "\\plugin");
	QDir d(pluginsDir);
	QFileInfoList l = d.entryInfoList(QDir::Dirs);
	foreach(const QFileInfo& info, l)
	{
		if (info.isDir())
		{
			QString configFilePath = QDir::toNativeSeparators(info.absoluteFilePath() + "\\config.ini");
			QSettings configSetting(configFilePath, QSettings::IniFormat);
			if (configSetting.status() == QSettings::NoError)
			{
				QString strKey = configSetting.value("key").toString();
				QString strName = configSetting.value("name").toString();
				QString strVer = configSetting.value("version").toString();
				QString strAuthor = configSetting.value("author").toString();
				Plugin::PluginType t = Plugin::getTypeFromStr(configSetting.value("type").toString());
				OmniObject::RespondType rt = configSetting.value("respondtype").toString() == "delay" ? 
					OmniObject::RespondType::Delay : OmniObject::RespondType::Real;
				switch (t)
				{
				case Plugin::PluginType::JS_SIMPLE:
				{
					JsSimplePlugin* p = new JsSimplePlugin(this, strName, strKey, strVer, strAuthor, info.filePath(), rt);
					m_plugins.insert(strKey, p);
					break;
				}
				case Plugin::PluginType::CPP_FREE:
				{
					CppFreePlugin* p = new CppFreePlugin(this, strName, strKey, strVer, strAuthor, info.filePath(), rt);
					m_plugins.insert(strKey, p);
					break;
				}
				case Plugin::PluginType::CPP_SIMPLELIST:
				{
					CppSimpleListPlugin* p = new CppSimpleListPlugin(this, strName, strKey, strVer, strAuthor, info.filePath(), rt);
					m_plugins.insert(strKey, p);
					break;
				}
				}
			}
		}
	}
}

void PluginManager::execQuery(Plugin* pPlugin, const QString& value)
{
	//根据类型选择对应widget
	m_stackedWidget->setCurrentWidget(pPlugin->type());
	m_stackedWidget->show();
	pPlugin->query(value, m_stackedWidget->widget(pPlugin->type()));
}

PluginStackedWidget::PluginStackedWidget(QWidget* parent)
	: QStackedWidget(parent)
	, m_omniPlugin(new OmniPlugin())
{
	m_pLabelPWidget = new LabelPluginWidget(this);
	m_pFreeWidget = new FreeWidget(this);
	m_pListWidget = new CppSimpleListWidget(this);

	addWidget(m_pLabelPWidget);
	addWidget(m_pFreeWidget);
	addWidget(m_pListWidget);

	m_omniPlugin->pluginManager()->setStackedWidget(this);
	connect(m_omniPlugin.data(), &OmniPlugin::startPluginQuery, m_omniPlugin->pluginManager(), &PluginManager::onStartQuery);
}

void PluginStackedWidget::setCurrentWidget(Plugin::PluginType t)
{
	switch (t)
	{
	case Plugin::PluginType::JS_SIMPLE:
		QStackedWidget::setCurrentWidget(m_pLabelPWidget);
		updateGeometry();
		break;
	case Plugin::PluginType::CPP_FREE:
		QStackedWidget::setCurrentWidget(m_pFreeWidget);
		updateGeometry();
		break;
	case Plugin::PluginType::CPP_SIMPLELIST:
		QStackedWidget::setCurrentWidget(m_pListWidget);
		updateGeometry();
		break;
	case Plugin::PluginType::UNKNOWN_TYPE:
		break;
	default:
		break;
	}
}

QWidget* PluginStackedWidget::widget(Plugin::PluginType t)
{
	QWidget* pWidget = currentWidget();
	switch (t)
	{
	case Plugin::PluginType::JS_SIMPLE:
		pWidget = m_pLabelPWidget;
		break;
	case Plugin::PluginType::CPP_FREE:
		pWidget = m_pFreeWidget;
		break;
	case Plugin::PluginType::CPP_SIMPLELIST:
		pWidget = m_pListWidget;
		break;
	case Plugin::PluginType::UNKNOWN_TYPE:
		break;
	default:
		break;
	}
	return pWidget;
}

void PluginStackedWidget::extend()
{
	if (!isVisible())
		return;

	PluginWidgetInterface* pWidget = dynamic_cast<PluginWidgetInterface*>(currentWidget());
	if (pWidget)
		pWidget->extend();
}


QSharedPointer<OmniPlugin> PluginStackedWidget::getOmniPlugin() const
{
	return m_omniPlugin;
}

bool PluginStackedWidget::eventFilter(QObject* o, QEvent* e)
{
	if (isVisible() && e->type() == QEvent::KeyPress)
	{
		QKeyEvent* ev = dynamic_cast<QKeyEvent*>(e);
		if (ev)
		{
			if (ev->key() == Qt::Key_Right && ev->modifiers() & Qt::AltModifier)
				extend();

			if (ev->key() == Qt::Key_Escape)
				hide();

			if (ev->key() == Qt::Key_Down)
				next();

			if (ev->key() == Qt::Key_Up)
				prev();

			if (ev->key() == Qt::Key_Return || ev->key() == Qt::Key_Enter)
			{
				shot();
				clear();
			}
		}
		
	}

	return QStackedWidget::eventFilter(o, e);
}

void PluginStackedWidget::next()
{
	PluginWidgetInterface* pWidget = dynamic_cast<PluginWidgetInterface*>(currentWidget());
	if (pWidget)
		pWidget->next();
}

void PluginStackedWidget::prev()
{
	PluginWidgetInterface* pWidget = dynamic_cast<PluginWidgetInterface*>(currentWidget());
	if (pWidget)
		pWidget->prev();
}

void PluginStackedWidget::shot()
{
	PluginWidgetInterface* pWidget = dynamic_cast<PluginWidgetInterface*>(currentWidget());
	if (pWidget)
		pWidget->shot();
}

void PluginStackedWidget::clear()
{
	hide();
	PluginWidgetInterface* pWidget = dynamic_cast<PluginWidgetInterface*>(currentWidget());
	if (pWidget)
		pWidget->clear();
}
