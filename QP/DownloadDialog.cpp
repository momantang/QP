#include "DownloadDialog.h"
#include "utils.h"
#include "Settings.h"

enum { TitleColum, DurationColum, TagColum };
class ContentTreeItem :public QTreeWidgetItem
{
public:
	static  constexpr  int SectionItemType = QTreeWidgetItem::UserType;
	static  constexpr  int ContentItemType = QTreeWidgetItem::UserType + 1;
	static  constexpr  int ItemRole = Qt::UserRole + 1;
private:
	ContentTreeItem(const Extractor::ContentItem& item) :QTreeWidgetItem(ContentItemType)
	{
		setText(TitleColum, item.title);
		setToolTip(TitleColum, item.title);
		QTreeWidgetItem::setData(0, ItemRole, item.id);
		QTreeWidgetItem::setData(0, Qt::CheckStateRole, Qt::Unchecked);
		if (item.flags & ContentItemFlag::Disabled)
		{
			setDisabled(true);
			setFlags(Qt::ItemNeverHasChildren);
		}
		else
		{
			setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren);
		}
		QString tag;
		if (item.flags & ContentItemFlag::VipOnly)
		{
			tag = QStringLiteral("会员");
		}
		else if (item.flags & ContentItemFlag::PayOnly)
		{
			tag = QStringLiteral("付费");
		}
		else if (item.flags & ContentItemFlag::AllowWaitFree)
		{
			tag = QStringLiteral("等免");
		}
		if (!tag.isEmpty())
		{
			setText(TagColum, tag);
			setForeground(TagColum, Qt::white);
			setBackground(TagColum, B23Style::Pink);
		}

		if (item.durationInSec > 0)
		{
			setText(DurationColum, utils::secs2HmsStr(item.durationInSec));
			setToolTip(DurationColum, utils::secs2HmsStrLocaleStr(item.durationInSec));
		}
	}
	ContentTreeItem(const Extractor::Section& section) :QTreeWidgetItem(SectionItemType)
	{
		setText(TitleColum, section.title);
		setChildIndicatorPolicy(ChildIndicatorPolicy::ShowIndicator);
		auto flags = Qt::ItemIsEnabled | Qt::ItemIsAutoTristate;
		if (section.episodes.size() != 0)
		{
			flags |= Qt::ItemIsUserCheckable;
			QTreeWidgetItem::setData(0, Qt::CheckStateRole, Qt::Unchecked);
		}
		setFlags(flags);

		bool allchildrenDisabled = true;
		for (auto& ep : section.episodes)
		{
			if (!(ep.flags & ContentItemFlag::Disabled))
			{
				allchildrenDisabled = false;
				break;
			}
		}
		if (allchildrenDisabled)
		{
			setDisabled(true);
		}
	}
public:
	QnList qnList;
	static QList<QTreeWidgetItem*> createItems(const QList<Extractor::ContentItem>& items)
	{
		QList<QTreeWidgetItem*> treeItems;
		treeItems.reserve(items.size());
		for (auto& item : items)
		{
			treeItems.append(new ContentTreeItem(item));
		}
		return treeItems;
	}
	static QList<QTreeWidgetItem*> createSectionItems(const QList<Extractor::Section>& sections)
	{
		QList<QTreeWidgetItem*> treeItems;
		treeItems.reserve(sections.size());
		for (auto& section : sections)
		{
			auto item = new ContentTreeItem(section);
			item->addChildren(createItems(section.episodes));
			treeItems.append(item);
		}
		return treeItems;

	}

	qint64 contentItemId()
	{
		return data(0, ItemRole).toLongLong();
	}
	void setChecked(bool checked)
	{
		QTreeWidgetItem::setData(0, Qt::CheckStateRole, checked ? Qt::Checked : Qt::Unchecked);
	}
	ContentTreeWidget* treeWidget()
	{
		return static_cast<ContentTreeWidget*>(QTreeWidgetItem::treeWidget());
	}
	QString longTitle()
	{
		// 如果 section/item 标题为"正片", 则视其为空
	   // 比如电影《星际穿越》, 它的标题为"星际穿越", 有两个section("正片"和"预告花絮"),
	   //     标题为"正片"的section包含一个标题为"正片"的视频
		auto clearIFZP = [](QString& s)
		{
			if (s == QStringLiteral("正片"))
			{
				s.clear();
			}
		};
		QString ret;
		if (parent() != nullptr)
		{
			auto sectionTitle = parent()->text(0);
			auto itemtitle = text(0);
			clearIFZP(sectionTitle);
			clearIFZP(itemtitle);
			if (sectionTitle.isEmpty())
			{
				ret = itemtitle;
			}
			else if (!itemtitle.isEmpty())
			{
				ret = sectionTitle + " " + itemtitle;
			}
			else
			{
				ret = text(0);
				clearIFZP(ret);
			}
			return utils::legalizedFileName(ret);
		}
	}
private:
	void setData(int column, int role, const QVariant& value) override
	{
		if (role == Qt::CheckStateRole)
		{
			if (type() == SectionItemType)
			{
				treeWidget()->beginSelMultItems();
				QTreeWidgetItem::setData(column, role, value);
				treeWidget()->endSelMultItems();
				return;
			}
			else if (Type() == ContentItemType)
			{
				if (!isDisabled())
				{
					setSelected(value == Qt::Checked);
				}
				return;
			}
		}
		QTreeWidgetItem::setData(column, role, value);
	}
};
const int ContentTreeWidget::RowSpacing=2;
ContentTreeWidget::ContentTreeWidget(const Extractor::Result* content, QWidget* parent):QTreeWidget(parent)
{
	Q_ASSERT(content->type!=ContentType::LIVE);

	setAllColumnsShowFocus(true);
	setDragDropMode(DragDropMode::NoDragDrop);
	setSelectionMode(QAbstractItemView::MultiSelection);
	setStyleSheet(QStringLiteral("QTreeView::item{margin-top:%1px;margin-bottom:%1px;}").arg(RowSpacing / 2));
	setMidLineWidth(fontMetrics().horizontalAdvance("+第001话 不可思议的魔法书+++1234:00:00 +++会员+"));

	setColumnCount(3);
	if (content->type==ContentType::COMIC)
	{
		setHeaderLabels();
	}
}

ContentTreeItem* ContentTreeWidget::currentItem() const
{
}

void ContentTreeWidget::selectAll()
{
	QTreeWidget::selectAll();
}

void ContentTreeWidget::beginSelMultItems()
{
}

void ContentTreeWidget::endSelMultItems()
{
}

QList<QTreeWidgetItem*> ContentTreeWidget::selection2Items(const QItemSelection& selection)
{
}

void ContentTreeWidget::drawRow(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	QTreeWidget::drawRow(painter, option, index);
}

void ContentTreeWidget::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	QTreeWidget::selectionChanged(selected, deselected);
}

DownloadDialog::DownloadDialog(const QString& url, QWidget* parent)
{
}

DownloadDialog::~DownloadDialog()
{
}

QList<AbstractDownloadTask*> DownloadDialog::getDownloadTasks()
{
}

void DownloadDialog::open()
{
	QDialog::open();
}

void DownloadDialog::abortExtract()
{
}

void DownloadDialog::extratorErrorOccured(const QString& errorString)
{
}

void DownloadDialog::setupUi()
{
}

void DownloadDialog::selAllBtnClicked()
{
}

void DownloadDialog::selCntChanged(int curr, int prev)
{
}

void DownloadDialog::startGetCurrentItemQnList()
{
}

void DownloadDialog::getCurrentItemQnListFinished()
{
}

void DownloadDialog::selectPath()
{
}

void DownloadDialog::updateQnComboBox(QnList qnList)
{
}

void DownloadDialog::addPlayDirect(QBoxLayout* layout)
{
}
