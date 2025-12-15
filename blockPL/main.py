import sys
from random import randint
from PyQt5.QtWidgets import (
    QApplication,
    QMainWindow,
    QGraphicsView,
    QGraphicsScene,
    QGraphicsItem,
    QGraphicsPathItem,
    QVBoxLayout,
    QHBoxLayout,
    QWidget,
    QPushButton,
    QLabel,
    QComboBox,
    QMessageBox,
)
from PyQt5.QtCore import Qt, QRectF
from PyQt5.QtGui import QPainter, QBrush, QPen, QColor, QFont, QPainterPath, QTransform


class Port(QGraphicsItem):
    def __init__(self, parent, is_output, port_index=0):
        super().__init__(parent)
        self.parent = parent
        self.is_output = is_output
        self.port_index = port_index
        self.connections = []
        self.setFlag(QGraphicsItem.ItemSendsScenePositionChanges)

    def boundingRect(self):
        return QRectF(-5, -5, 10, 10)

    def paint(self, painter, option, widget):
        painter.setBrush(QBrush(QColor(100, 150, 200)))
        painter.setPen(QPen(Qt.black, 1))
        painter.drawEllipse(self.boundingRect())

    def itemChange(self, change, value):
        if change == QGraphicsItem.ItemScenePositionHasChanged:
            for connection in self.connections:
                connection.updatePath()
        return super().itemChange(change, value)


class Connection(QGraphicsPathItem):
    def __init__(self, start_port, end_port):
        super().__init__()
        self.start_port = start_port
        self.end_port = end_port
        self.start_port.connections.append(self)
        self.end_port.connections.append(self)
        self.setPen(QPen(Qt.darkGray, 2))
        self.setZValue(-1)
        self.updatePath()

    def updatePath(self):
        path = QPainterPath()
        start_pos = self.start_port.scenePos()
        end_pos = self.end_port.scenePos()

        path.moveTo(start_pos)
        dx = (end_pos.x() - start_pos.x()) * 0.5
        path.cubicTo(
            start_pos.x() + dx,
            start_pos.y(),
            end_pos.x() - dx,
            end_pos.y(),
            end_pos.x(),
            end_pos.y(),
        )
        self.setPath(path)

    def remove(self):
        if self in self.start_port.connections:
            self.start_port.connections.remove(self)
        if self in self.end_port.connections:
            self.end_port.connections.remove(self)
        if self.scene():
            self.scene().removeItem(self)


class Node(QGraphicsItem):
    NODE_TYPES = {
        "Input": {"inputs": 0, "color": QColor(255, 182, 193)},  # Light Pink
        "Result": {"inputs": 1, "color": QColor(216, 191, 216)},  # Thistle
        "Xor": {"inputs": 2, "color": QColor(135, 206, 250)},  # Light Sky Blue
        "Concat": {"inputs": 2, "color": QColor(152, 251, 152)},  # Pale Green
        "Byte sum": {"inputs": 2, "color": QColor(255, 160, 122)},  # Light Salmon
        "Sum": {"inputs": 2, "color": QColor(221, 160, 221)},  # Plum
        "Sha-256": {"inputs": 1, "color": QColor(240, 230, 140)},  # Khaki
        "Sha-384": {"inputs": 1, "color": QColor(176, 224, 230)},  # Powder Blue
        "Sha-512": {"inputs": 1, "color": QColor(255, 218, 185)},  # Peach Puff
        "Md5": {"inputs": 1, "color": QColor(245, 222, 179)},  # Wheat
    }

    def __init__(self, node_type, value=""):
        super().__init__()
        self.node_type = node_type
        self.value = value
        self.input_ports = []
        self.output_port = None
        self.width = 120
        self.height = 60

        self.setFlag(QGraphicsItem.ItemIsMovable)
        self.setFlag(QGraphicsItem.ItemIsSelectable)
        self.setFlag(QGraphicsItem.ItemSendsScenePositionChanges)

        self.setupPorts()
        self.updatePortPositions()

    def setupPorts(self):
        input_count = self.NODE_TYPES[self.node_type]["inputs"]
        for i in range(input_count):
            port = Port(self, False, i)
            self.input_ports.append(port)

        if self.node_type != "Result":
            self.output_port = Port(self, True)

    def updatePortPositions(self):
        input_count = len(self.input_ports)
        for i, port in enumerate(self.input_ports):
            y_pos = (i + 1) * self.height / (input_count + 1)
            port.setPos(-5, y_pos)

        if self.output_port:
            self.output_port.setPos(self.width + 5, self.height / 2)

    def boundingRect(self):
        return QRectF(0, 0, self.width, self.height)

    def paint(self, painter, option, widget):
        color = self.NODE_TYPES[self.node_type]["color"]
        painter.setBrush(QBrush(color.lighter(150) if self.isSelected() else color))
        painter.setPen(QPen(Qt.black, 2 if self.isSelected() else 1))
        painter.drawRoundedRect(0, 0, self.width, self.height, 10, 10)

        painter.setPen(QPen(Qt.black))
        painter.setFont(QFont("Arial", 10))

        text = self.node_type

        painter.drawText(self.boundingRect(), Qt.AlignCenter, text)

    def itemChange(self, change, value):
        if change == QGraphicsItem.ItemPositionHasChanged:
            self.updatePortPositions()
        return super().itemChange(change, value)


class GraphicsScene(QGraphicsScene):
    def __init__(self):
        super().__init__(-1000, -1000, 2000, 2000)
        self.start_connection_port = None
        self.temp_connection = None

    def mousePressEvent(self, event):
        item = self.itemAt(event.scenePos(), QTransform())

        if isinstance(item, Port):
            self.start_connection_port = item
            self.temp_connection = QGraphicsPathItem()
            self.temp_connection.setPen(QPen(Qt.darkGray, 2, Qt.DashLine))
            self.addItem(self.temp_connection)
            self.updateTempConnection(event.scenePos())
            event.accept()
        else:
            super().mousePressEvent(event)

    def mouseMoveEvent(self, event):
        if self.temp_connection:
            self.updateTempConnection(event.scenePos())
            event.accept()
        else:
            super().mouseMoveEvent(event)

    def mouseReleaseEvent(self, event):
        if self.temp_connection:
            self.removeItem(self.temp_connection)
            self.temp_connection = None

            item = self.itemAt(event.scenePos(), QTransform())

            if (
                isinstance(item, Port)
                and item != self.start_connection_port
                and item.is_output != self.start_connection_port.is_output
            ):
                if self.start_connection_port.is_output and not item.is_output:
                    can_connect = True
                    for conn in item.connections:
                        if conn.end_port == item:
                            can_connect = False
                            break

                    if can_connect:
                        connection = Connection(self.start_connection_port, item)
                        self.addItem(connection)

            self.start_connection_port = None
            event.accept()
        else:
            super().mouseReleaseEvent(event)

    def updateTempConnection(self, end_pos):
        if self.start_connection_port and self.temp_connection:
            path = QPainterPath()
            start_pos = self.start_connection_port.scenePos()

            path.moveTo(start_pos)
            dx = (end_pos.x() - start_pos.x()) * 0.5
            path.cubicTo(
                start_pos.x() + dx,
                start_pos.y(),
                end_pos.x() - dx,
                end_pos.y(),
                end_pos.x(),
                end_pos.y(),
            )
            self.temp_connection.setPath(path)


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Графический конструктор команд")
        self.setGeometry(100, 100, 1200, 800)

        self.setupUI()
        self.addNode("Input")
        self.addNode("Result")

    def setupUI(self):
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        layout = QHBoxLayout(central_widget)

        tool_widget = QWidget()
        tool_layout = QVBoxLayout(tool_widget)
        tool_layout.setAlignment(Qt.AlignTop)

        tool_layout.addWidget(QLabel("Тип блока:"))
        self.node_type_combo = QComboBox()
        self.node_type_combo.addItems(list(Node.NODE_TYPES.keys())[2:])
        tool_layout.addWidget(self.node_type_combo)

        self.add_node_btn = QPushButton("Добавить блок")
        self.add_node_btn.clicked.connect(self.addNode)
        tool_layout.addWidget(self.add_node_btn)

        self.execute_btn = QPushButton("Скомпилировать программу")
        self.execute_btn.clicked.connect(self.executeProgram)
        tool_layout.addWidget(self.execute_btn)

        self.clear_btn = QPushButton("Очистить сцену")
        self.clear_btn.clicked.connect(self.clearScene)
        tool_layout.addWidget(self.clear_btn)

        self.delete_btn = QPushButton("Удалить выделенное")
        self.delete_btn.clicked.connect(self.deleteSelected)
        tool_layout.addWidget(self.delete_btn)

        self.scene = GraphicsScene()
        self.view = QGraphicsView(self.scene)
        self.view.setRenderHint(QPainter.Antialiasing)

        layout.addWidget(tool_widget, 1)
        layout.addWidget(self.view, 4)

    def addNode(self, node_type=None):
        if not node_type:
            node_type = self.node_type_combo.currentText()
        node = Node(node_type, "")

        view_center = self.view.mapToScene(self.view.viewport().rect().center())
        node.setPos(
            view_center.x() - node.width / 2 + randint(-10, 10) * node.width / 10,
            view_center.y() - node.height / 2 + randint(-10, 10) * node.height / 10,
        )

        self.scene.addItem(node)

    def GetInitStr(self, varN):
        return f"""cl_mem buf_v{varN} = clCreateBuffer(context, CL_MEM_READ_WRITE, \
input_data.size() * sizeof(cl_uint), nullptr, &err);
cl_mem buf_v{varN}_size = clCreateBuffer(context, CL_MEM_READ_WRITE, \
size_data.size() * sizeof(cl_uint), nullptr, &err);
"""

    def GetExeBlock(self, t, vars):
        print(t)
        if t == "Input":
            return f"""err = clEnqueueWriteBuffer(queue, buf_v{vars[0]}, CL_TRUE, 0,
                                input_data.size() * sizeof(cl_uint), input_data.data(),
                                0, nullptr, nullptr);
size_data[0]=data_size;
err = clEnqueueWriteBuffer(queue, buf_v{vars[0]}_size, CL_TRUE, 0,
                                size_data.size() * sizeof(cl_uint), size_data.data(),
                                0, nullptr, nullptr);"""
        elif t == "Sha-512":
            return f"""clSetKernelArg(kernel_sha512, 0, sizeof(cl_mem), &buf_v{vars[0]});
clSetKernelArg(kernel_sha512, 1, sizeof(cl_mem), &buf_v{vars[0]}_size);
clSetKernelArg(kernel_sha512, 2, sizeof(cl_mem), &buf_v{vars[1]});
clSetKernelArg(kernel_sha512, 3, sizeof(cl_mem), &buf_v{vars[1]}_size);
clEnqueueNDRangeKernel(queue, kernel_sha512, 1, nullptr, &global_size, nullptr, 0, nullptr, nullptr);"""
        elif t == "Md5":
            return f"""clSetKernelArg(kernel_md5, 0, sizeof(cl_mem), &buf_v{vars[0]});
clSetKernelArg(kernel_md5, 1, sizeof(cl_mem), &buf_v{vars[0]}_size);
clSetKernelArg(kernel_md5, 2, sizeof(cl_mem), &buf_v{vars[1]});
clSetKernelArg(kernel_md5, 3, sizeof(cl_mem), &buf_v{vars[1]}_size);
clEnqueueNDRangeKernel(queue, kernel_md5, 1, nullptr, &global_size, nullptr, 0, nullptr, nullptr);"""
        elif t == "Xor":
            return f"""clSetKernelArg(kernel_xor, 0, sizeof(cl_mem), &buf_v{vars[0]});
clSetKernelArg(kernel_xor, 1, sizeof(cl_mem), &buf_v{vars[0]}_size);
clSetKernelArg(kernel_xor, 2, sizeof(cl_mem), &buf_v{vars[1]});
clSetKernelArg(kernel_xor, 3, sizeof(cl_mem), &buf_v{vars[1]}_size);
clSetKernelArg(kernel_xor, 4, sizeof(cl_mem), &buf_v{vars[2]});
clSetKernelArg(kernel_xor, 5, sizeof(cl_mem), &buf_v{vars[2]}_size);
clEnqueueNDRangeKernel(queue, kernel_xor, 1, nullptr, &global_size, nullptr, 0, nullptr, nullptr);"""
        elif t == "Result":
            return f"""clEnqueueReadBuffer(queue, buf_v{vars[0]}, CL_TRUE, 0,
                                output_data.size() * sizeof(cl_uint), output_data.data(),
                                0, nullptr, nullptr);
clEnqueueReadBuffer(queue, buf_v{vars[0]}_size, CL_TRUE, 0,
                                size_data.size() * sizeof(cl_uint), size_data.data(),
                                0, nullptr, nullptr);"""
        else:
            return f"Wrong type {t}\n"

    def executeProgram(self):
        nodes = [item for item in self.scene.items() if isinstance(item, Node)]

        if not nodes:
            QMessageBox.information(self, "Выполнение", "На сцене нет блоков!")
            return

        for i, node in enumerate(nodes):
            node.id = i
            node.inputs = []
        for node in nodes:
            for j, port in enumerate(node.input_ports):
                for connection in port.connections:
                    if connection.end_port == port:
                        source_node = connection.start_port.parent
                        node.inputs += [source_node.id]
                        break

        execution_order = self.calculateExecutionOrder(nodes)

        init_str = ""
        exec_str = ""
        for i, node in enumerate(execution_order):
            input_str = "()"

            init_str += self.GetInitStr(node.id)

            if node.inputs:
                input_str = "(var-" + " var-".join([str(i) for i in node.inputs]) + ")"
            if node.node_type == "Result":
                print(f"{node.node_type}, {input_str}")
                exec_str += self.GetExeBlock(node.node_type, node.inputs)
            else:
                print(f"{node.node_type}, {input_str}, (var-{node.id})")
                exec_str += self.GetExeBlock(node.node_type, node.inputs + [node.id])

        print()

        out_f = open("main.cpp", "w", encoding="utf-8")

        with open("main.cpp_1.txt", "r", encoding="utf-8") as f:
            print(f.read(), file=out_f)
        print(init_str, file=out_f)
        with open("main.cpp_2.txt", "r", encoding="utf-8") as f:
            print(f.read(), file=out_f)
        print(exec_str, file=out_f)
        with open("main.cpp_3.txt", "r", encoding="utf-8") as f:
            print(f.read(), file=out_f)

        out_f.close()

        QMessageBox.information(
            self,
            "Выполнение",
            "Информация о программе выведена в консоль!\n\n"
            + f"Всего блоков: {len(nodes)}\n"
            + "Порядок выполнения определен.",
        )

    def calculateExecutionOrder(self, nodes):
        graph = {}
        node_to_id = {node: i for i, node in enumerate(nodes)}

        for node in nodes:
            graph[node_to_id[node]] = []

        for node in nodes:
            for port in node.input_ports:
                for connection in port.connections:
                    if connection.end_port == port:
                        source_node = connection.start_port.parent
                        graph[node_to_id[node]].append(node_to_id[source_node])

        visited = [False] * len(nodes)
        temp = [False] * len(nodes)
        order = []

        def visit(node_id):
            if temp[node_id]:
                return False
            if not visited[node_id]:
                temp[node_id] = True
                for dependency in graph[node_id]:
                    if not visit(dependency):
                        return False
                visited[node_id] = True
                temp[node_id] = False
                order.append(nodes[node_id])
            return True

        for node_id in range(len(nodes)):
            if not visited[node_id]:
                if not visit(node_id):
                    print(
                        "Обнаружен цикл в графе! Невозможно определить порядок выполнения."
                    )
                    return None

        return order

    def clearScene(self):
        reply = QMessageBox.question(
            self,
            "Очистка",
            "Вы уверены, что хотите очистить сцену?",
            QMessageBox.Yes | QMessageBox.No,
        )
        if reply == QMessageBox.Yes:
            self.scene.clear()
            self.scene = GraphicsScene()
            self.view.setScene(self.scene)
            self.addNode("Input")
            self.addNode("Result")

    def deleteSelected(self):
        for item in self.scene.selectedItems():
            if isinstance(item, Node):
                for port in item.input_ports + [item.output_port]:
                    for connection in port.connections[:]:
                        connection.remove()
                self.scene.removeItem(item)
            elif isinstance(item, Connection):
                item.remove()


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())
