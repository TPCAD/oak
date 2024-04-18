# Disk

这里的硬盘特指机械硬盘。

## 物理结构

<img src="https://upload.wikimedia.org/wikipedia/commons/0/02/Cylinder_Head_Sector.svg" style="zoom: 60%">

机械硬盘最主要的结构是盘片（Platter）和磁头（Head）。盘片由磁性材料制成，磁头在
通过改变盘片区域的磁性来存储数据。盘片旋转时，若磁头保持在一个位置上，则每个磁
头会在磁盘表面划出一个圆形轨迹，称为磁道（Track）。不同盘片的相同磁道会构成一个
圆柱面，称为柱面（Cylinder）。将圆形盘片分成若干相同大小的扇形区域，称为扇区
（Sector）。通过 Cylinder-Head-Sector 就可以定位一块数据的位置。因此，硬盘读写
的最小单位是一个扇区。
