import copy
import math
import pickle
import unittest

from iixml_matrix import Matrix, MatrixError


class MatrixTests(unittest.TestCase):
    def test_shape_values_and_arithmetic(self):
        a = Matrix((2, 2), [1, 2, 3, 4])
        self.assertEqual(a.shape, (2, 2))
        self.assertEqual(a.at((1, 0)), 3)
        self.assertEqual(a.reshape((4,)).values, [1, 2, 3, 4])
        self.assertEqual((a / 2).values, [0.5, 1, 1.5, 2])
        self.assertEqual((10 - a).values, [9, 8, 7, 6])
        self.assertEqual((a * 2).values, [2, 4, 6, 8])
        self.assertEqual(a.sum(), 10)
        self.assertEqual(a.mean(), 2.5)
        self.assertEqual((a @ Matrix((2, 1), [5, 6])).values, [17, 39])
        self.assertEqual(a.values, [1, 2, 3, 4])

    def test_xml_and_types(self):
        a = Matrix((2,), [1, 1.0])
        b = Matrix.from_xml(a.to_xml())
        self.assertEqual(b.shape, (2,))
        self.assertIs(type(b.values[0]), int)
        self.assertIs(type(b.values[1]), float)
        self.assertEqual(Matrix.from_xml("<matrix shape='2'><value>+2</value><value>3e-1</value></matrix>").values, [2, 0.3])
        self.assertEqual(Matrix((0, 2), []).sum(), 0)
        self.assertIs(copy.copy(a), a)
        self.assertIs(copy.deepcopy(a), a)
        self.assertEqual(pickle.loads(pickle.dumps(a)).values, [1, 1.0])
        with self.assertRaises(RuntimeError):
            a.__init__((1,), [5])
        self.assertEqual(a.values, [1, 1.0])

    def test_batched_matmul_and_vectors(self):
        left = Matrix((2, 1, 2, 3), range(1, 13))
        right = Matrix((1, 2, 3, 2), [1, 0, 0, 1, 1, 1, 2, 0, 0, 2, 1, -1])
        result = left @ right
        self.assertEqual(result.shape, (2, 2, 2, 2))
        self.assertEqual(result.values, [4, 5, 10, 11, 5, 1, 14, 4,
                                         16, 17, 22, 23, 23, 7, 32, 10])
        vector = Matrix((3,), [1, 2, 3])
        dot = vector @ Matrix((3,), [4, 5, 6])
        self.assertEqual((dot.shape, dot.values), ((), [32]))
        self.assertEqual((vector @ Matrix((3, 2), [1, 0, 0, 1, 1, 1])).values, [4, 5])
        vector_batches = vector @ Matrix((2, 3, 2), [1, 0, 0, 1, 1, 1, 2, 0, 0, 2, 1, -1])
        self.assertEqual((vector_batches.shape, vector_batches.values), ((2, 2), [4, 5, 5, 1]))
        self.assertEqual((left @ vector).shape, (2, 1, 2))
        self.assertEqual((left @ vector).values, [14, 32, 50, 68])
        self.assertEqual((Matrix((2, 0), []) @ Matrix((0, 3), [])).values, [0] * 6)
        self.assertEqual((Matrix((0, 2, 3), []) @ Matrix((1, 3, 4), range(12))).shape, (0, 2, 4))
        with self.assertRaises(MatrixError):
            Matrix((2, 2, 3), range(12)) @ Matrix((3, 3, 2), range(18))
        with self.assertRaises(MatrixError):
            Matrix((), [1]) @ vector
        with self.assertRaises(MatrixError):
            Matrix((1, 1, 1), [2**63 - 1]) @ Matrix((1, 1, 1), [2])

    def test_axis_reductions(self):
        matrix = Matrix((2, 3, 4), range(1, 25))
        self.assertEqual(matrix.sum(axis=0).shape, (3, 4))
        self.assertEqual(matrix.sum(axis=0).values, list(range(14, 38, 2)))
        rows = matrix.sum(axis=-1, keepdims=True)
        self.assertEqual((rows.shape, rows.values), ((2, 3, 1), [10, 26, 42, 58, 74, 90]))
        averages = matrix.mean(axis=(0, 2))
        self.assertEqual((averages.shape, averages.values), ((3,), [8.5, 12.5, 16.5]))
        self.assertEqual(matrix.mean(axis=(0, 2), keepdims=True).shape, (1, 3, 1))
        self.assertEqual(matrix.sum(axis=()).values, matrix.values)
        self.assertEqual((matrix.sum(axis=(0, 1, 2)).shape, matrix.sum(axis=(0, 1, 2)).values), ((), [300]))
        self.assertEqual((matrix.sum(keepdims=True).shape, matrix.sum(keepdims=True).values), ((1, 1, 1), [300]))
        self.assertEqual(matrix.mean(), 12.5)
        self.assertEqual(matrix.mean(keepdims=True).values, [12.5])
        empty = Matrix((2, 0, 3), [])
        self.assertEqual((empty.sum(axis=1).shape, empty.sum(axis=1).values), ((2, 3), [0] * 6))
        with self.assertRaises(MatrixError):
            empty.mean(axis=1)
        self.assertEqual(Matrix((0, 0), []).mean(axis=0).shape, (0,))
        self.assertEqual(Matrix((0, 2), []).mean(axis=()).shape, (0, 2))
        with self.assertRaises(MatrixError):
            matrix.sum(axis=(0, -3))
        with self.assertRaises(MatrixError):
            matrix.mean(axis=3)
        with self.assertRaises(TypeError):
            matrix.sum(axis=True)
        with self.assertRaises(MatrixError):
            Matrix((2,), [2**63 - 1, 1]).sum(axis=0)
        with self.assertRaises(MatrixError):
            matrix.mean(axis=-(2**63))

    def test_tensordot(self):
        left = Matrix((2, 2, 2), range(1, 9))
        right = Matrix((2, 2, 2), [1, 0, 0, 1, 1, 1, 2, 0])
        product = left.tensordot(right, (1, 2), (0, 1))
        self.assertEqual((product.shape, product.values), ((2, 2), [12, 5, 28, 13]))
        self.assertEqual(left.tensordot(right, (-2, -1), (0, 1)).values, product.values)
        xml_result = Matrix.from_xml(product.to_xml())
        self.assertEqual((xml_result.shape, xml_result.values), (product.shape, product.values))
        lhs = Matrix((2, 3, 4), range(1, 25))
        rhs = Matrix((4, 5, 2), range(1, 41))
        reordered = lhs.tensordot(rhs, (2, 0), (0, 2))
        expected = [sum(lhs.at((d, i, k)) * rhs.at((k, j, d)) for k in range(4) for d in range(2))
                    for i in range(3) for j in range(5)]
        self.assertEqual((reordered.shape, reordered.values), ((3, 5), expected))
        outer = Matrix((2,), [2, 3]).tensordot(Matrix((3,), [4, 5, 6]), (), ())
        self.assertEqual((outer.shape, outer.values), ((2, 3), [8, 10, 12, 12, 15, 18]))
        scalar = Matrix((2,), [2, 3]).tensordot(Matrix((2,), [4, 5]), 0, 0)
        self.assertEqual((scalar.shape, scalar.values), ((), [23]))
        scalar_outer = Matrix((), [2]).tensordot(Matrix((), [3]), (), ())
        self.assertEqual((scalar_outer.shape, scalar_outer.values), ((), [6]))
        empty_dot = Matrix((0,), []).tensordot(Matrix((0,), []), 0, 0)
        self.assertEqual((empty_dot.shape, empty_dot.values), ((), [0]))
        self.assertEqual(Matrix((2, 0), []).tensordot(Matrix((0, 3), []), 1, 0).values, [0] * 6)
        with self.assertRaises(MatrixError):
            left.tensordot(right, (0, 0), (0, 1))
        with self.assertRaises(MatrixError):
            left.tensordot(right, (0,), (0, 1))
        with self.assertRaises(MatrixError):
            left.tensordot(Matrix((3,), [1, 2, 3]), 0, 0)
        with self.assertRaises(MatrixError):
            Matrix((1,), [2**63 - 1]).tensordot(Matrix((1,), [2]), 0, 0)

    def test_rejections(self):
        with self.assertRaises((TypeError, ValueError)):
            Matrix((1,), [True])
        with self.assertRaises((TypeError, ValueError)):
            Matrix((True,), [1])
        with self.assertRaises(OverflowError):
            Matrix((1,), [1 << 63])
        with self.assertRaises(MatrixError):
            Matrix((1,), [math.inf])
        with self.assertRaises(MatrixError):
            Matrix((2,), [1])
        with self.assertRaises(MatrixError):
            Matrix.from_xml("<matrix shape='1'><value>false</value></matrix>")
        with self.assertRaises(MatrixError):
            Matrix((1,), [1]) / 0
        with self.assertRaises(MatrixError):
            Matrix((1,), [2**53 + 1]) + 1.0
        with self.assertRaises(MatrixError):
            Matrix((0,), []).mean()
        with self.assertRaises(MatrixError):
            Matrix((1,), [1]).at((1,))


if __name__ == "__main__":
    unittest.main()
